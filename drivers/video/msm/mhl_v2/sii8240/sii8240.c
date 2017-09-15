/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Author: kmini.park <kmini.park@samsung.com>
 *            Sangmi Park <sm0327.park@samsung.com>
 * Date: 2:06 PM, 1st June,2012
 *
 * Based on drivers/video/sii9234.c[Google AOSP(Galaxy Nexus/Tuna)]
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

#include <linux/err.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/qpnp/pin.h>
#include <linux/sii8240.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <video/edid.h>
#include <linux/input.h>
#include "sii8240_rcp.h"
#include "sii8240_platform.h"
#include "sii8240_driver.h"
/*#include <linux/barcode_emul.h>*/
#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#ifdef SII8240_CHECK_MONITOR
#include <mach/scm.h>
#endif


#undef pr_debug
#define pr_debug pr_info

#undef dev_dbg
#define dev_dbg dev_info
#endif

#define CONFIG_MHL_SWING_LEVEL 1

#ifdef CONFIG_EXTCON
static struct sec_mhl_cable support_cable_list[] = {
	{ .cable_type = EXTCON_MHL, },
	{ .cable_type = EXTCON_MHL_VB, },
	{ .cable_type = EXTCON_SMARTDOCK, },
};
#endif

static struct device *sii8240_mhldev;
static struct sii8240_data *g_sii8240;
#ifdef SII8240_CHECK_MONITOR
static struct hdcp_auth_status g_monitor_cmd;
#endif

struct class *sec_mhl;
EXPORT_SYMBOL(sec_mhl);

#ifdef SII8240_CHECK_MONITOR
static int sii8240_scm_call(struct sii8240_data *sii8240, u32 svc_id, u32 cmd_id,
		const void *cmd_buf, size_t cmd_len, void *resp_buf, size_t resp_len)
{
	int ret = 0;
	if (sii8240->ckdt_stable)
		ret = scm_call(svc_id, cmd_id, cmd_buf, cmd_len, resp_buf, resp_len);
	return ret;
}
#endif

static int mhl_write_byte_reg(struct i2c_client *client, u32 offset,
			u8 value)
{
	int ret = i2c_smbus_write_byte_data(client, offset, value);
	if (unlikely(ret < 0))
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);
	return ret;
}

static int mhl_read_byte_reg(struct i2c_client *client, unsigned int offset,
			u8 *value)
{
	int ret;

	if (!value)
		return -EINVAL;

	ret =  i2c_smbus_read_byte_data(client, offset);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);
		return ret;
	}

	*value = ret & 0x000000FF;
	return 0;
}

static int mhl_write_block_reg(struct i2c_client *client, unsigned int offset,
				u8 len, u8 *values)
{
	int ret;

	if (!values)
		return -EINVAL;

	ret = i2c_smbus_write_i2c_block_data(client, offset, len, values);
	if (unlikely(ret < 0))
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);
	return ret;
}

static int mhl_read_block_reg(struct i2c_client *client, unsigned int offset,
				u8 len, u8 *values)
{
	int ret;

	if (!values)
		return -EINVAL;
	ret = i2c_smbus_read_i2c_block_data(client, offset, len, values);
	if (unlikely(ret < 0))
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);

	return ret;
}
static int mhl_modify_reg(struct i2c_client *i2c_client, u8 offset,
	u8 mask, u8 data)
{
	u8 rd;
	int ret;

	ret = mhl_read_byte_reg(i2c_client, offset, &rd);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);
		return ret;
	}

	rd &= ~mask;
	rd |= (data & mask);

	ret = mhl_write_byte_reg(i2c_client, offset, rd);

	if (unlikely(ret < 0))
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);

	return ret;
}

/* NOTE: Registers are set and cleared either by 1 or 0.
 * Functions calling these mhl_set_reg and mhl_clear_reg should
 * take care of these register-specific details and adjust the mask
 */
static int mhl_clear_reg(struct i2c_client *client, unsigned int offset,
			u8 mask)
{
	int ret;
	u8 value;

	ret = mhl_read_byte_reg(client, offset, &value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);
		return ret;
	}
	value &= ~mask;
	ret = mhl_write_byte_reg(client, offset, value);
	if (unlikely(ret < 0))
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);

	return ret;
}

static int mhl_set_reg(struct i2c_client *client, unsigned int offset,
			u8 mask)
{
	int ret;

	u8 value;
	ret = mhl_read_byte_reg(client, offset, &value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);
		return ret;
	}

	value |= mask;
	ret = mhl_write_byte_reg(client, offset, value);
	if (unlikely(ret < 0))
		pr_err("[ERROR] sii8240: %s():%d offset:0x%X ret:%d\n",
				__func__, __LINE__, offset, ret);

	return ret;
}

#ifdef SII8240_CHECK_MONITOR
static void sii8240_link_monitor_timer(unsigned long data)
{
	struct sii8240_data *sii8240;

	sii8240 = dev_get_drvdata(sii8240_mhldev);
	queue_work(sii8240->mhl_link_monitor_wq, &sii8240->mhl_link_monitor_work);
}
static void sii8240_link_monitor_work(struct work_struct *work)
{
	int ret;
	unsigned char rd_data = 0, hdcp_query = 0, status = 0;
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
	unsigned char rd_data2 = 0;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	if (sii8240->state < STATE_MHL_DISCOVERY_SUCCESS) {
		g_monitor_cmd.a |= 0x08;
		g_monitor_cmd.a &= ~(0x01);
		del_timer_sync(&sii8240->mhl_timer);
		sii8240_scm_call(sii8240, _SCM_SVC_OEM, _SCM_OEM_CMD, &g_monitor_cmd, sizeof(g_monitor_cmd), NULL, 0);
		pr_info("%s() g_monitor_cmd.a = %d\n", __func__, g_monitor_cmd.a);
		pr_info("%s() : mhl status = %d\n", __func__, sii8240->state);
		return;
	}

	ret = mhl_read_byte_reg(tpi, TPI_HDCP_QUERY_DATA_REG, &hdcp_query);
	if (unlikely(ret < 0)) {
		pr_err ("[ERROR] %s() mhl already has been shut down\n", __func__);
		return;
	}

	rd_data = hdcp_query & HDCP_REPEATER_MASK;

	if (rd_data) {
		status = hdcp_query & EXTENDED_LINK_PROTECTION_MASK;
		pr_info("%s() repeater extended = %d\n", __func__, status);
	} else {
		status = hdcp_query & LOCAL_LINK_PROTECTION_MASK;
		pr_info("%s() local = %d\n", __func__, status);
	}
	if (status == 0) {
		g_monitor_cmd.a |= 0x04;
		sii8240_scm_call(sii8240, _SCM_SVC_OEM, _SCM_OEM_CMD, &g_monitor_cmd, sizeof(g_monitor_cmd), NULL, 0);
		pr_info("%s() g_monitor_cmd.a = %d\n", __func__, g_monitor_cmd.a);
	} else {
		g_monitor_cmd.a |= 0x02;
		sii8240_scm_call(sii8240, _SCM_SVC_OEM, _SCM_OEM_CMD, &g_monitor_cmd, sizeof(g_monitor_cmd), NULL, 0);
		pr_info("%s() g_monitor_cmd.a = %d\n", __func__, g_monitor_cmd.a);
	}

	/* check if AV_SET_MUTE is coming from QC (BIT6 of PAGE_2:0xA0 register) */
	ret = mhl_read_byte_reg(hdmi, 0xA0, &rd_data2);
	if (ret < 0)
		pr_err("[ERROR]sii8240: %s():%d failed !\n", __func__, __LINE__);
	else
		pr_info("sii8240: %s(): [jgk] PAGE_2:0xA0:0x%02x !\n", __func__, rd_data2);

	mod_timer(&sii8240->mhl_timer, jiffies + msecs_to_jiffies(1000));
}
#endif

static int set_mute_mode(struct sii8240_data *sii8240, bool mute)
{
	int ret;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	pr_info("set_mute_mode : %d\n", mute);
	if (mute) {
		ret = mhl_modify_reg(tpi, 0x1A, AV_MUTE_MASK, AV_MUTE_MASK);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
	} else {
		ret = mhl_modify_reg(tpi, 0x1A, AV_MUTE_MASK, AV_MUTE_NORMAL);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
	}

	return ret;
}
static int sii8240_send_avi_infoframe(struct sii8240_data *sii8240)
{
	int ret = 0;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	pr_info("sii8240: send avi infoframe %d\n", sii8240->hdmi_sink);
	if (sii8240->hdmi_sink)
		ret = mhl_write_block_reg(tpi, 0x0C, SIZE_AVI_INFOFRAME,
				sii8240->output_avi_data);
	return ret;
}
#ifdef SFEATURE_HDCP_SUPPORT
void mhl_ddc_bypass(bool bypass_on)
{
	int ret = 0;
	struct i2c_client *tmds = g_sii8240->pdata->tmds_client;

	pr_info("sii8240: ddc bypass - %s\n", bypass_on ? "on" : "off");

	if (bypass_on == true) {
		ret = mhl_set_reg(tmds, DCTL_REG, EXT_DDC_SEL);
		if (unlikely(ret < 0))
			pr_err("sii8240: %s():%d Fail to set register\n",
							__func__, __LINE__);
	} else {
		ret = mhl_clear_reg(tmds, DCTL_REG, EXT_DDC_SEL);
		if (unlikely(ret < 0))
			pr_err("sii8240: %s():%d Fail to set register\n",
				__func__, __LINE__);
	}
}

static int sii8240_hdcp_on(struct sii8240_data *sii8240, bool hdcp_on)
{
	int ret = 0;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	if (hdcp_on) {
		pr_info("sii8240: HDCP On\n");
#ifdef CONFIG_ARCH_MSM8974
		/* execute AP HDCP if HW rev > 06 and no KS01*/
		if (sii8240->pdata->drm_workaround)
			platform_ap_hdmi_hdcp_auth(sii8240);
#endif
		ret = mhl_modify_reg(tpi, HDCP_CTRL,
			BIT_TPI_HDCP_CONTROL_DATA_DOUBLE_RI_CHECK_MASK |
			BIT_TPI_HDCP_CONTROL_DATA_COPP_PROTLEVEL_MASK,
			BIT_TPI_HDCP_CONTROL_DATA_DOUBLE_RI_CHECK_ENABLE |
			BIT_TPI_HDCP_CONTROL_DATA_COPP_PROTLEVEL_MAX);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() unable to set HDCP_CTRL register\n", __func__);
			return ret;
		}
#ifdef SII8240_CHECK_MONITOR
		g_monitor_cmd.a = 0x01;
		g_monitor_cmd.b = NULL;
		g_monitor_cmd.c = NULL;
		g_monitor_cmd.d = 0;
		sii8240_scm_call(sii8240, _SCM_SVC_OEM, _SCM_OEM_CMD, &g_monitor_cmd, sizeof(g_monitor_cmd), NULL, 0);
		pr_info("%s() g_monitor_cmd.a = %d\n", __func__, g_monitor_cmd.a);
#endif

	} else {
		pr_info("sii8240:HDCP Off\n");
		ret = mhl_write_byte_reg(tpi, HDCP_CTRL, 0x00);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] unable to reset HDCP_CTRL register to 0x00\n");
			return ret;
		}
#ifdef SII8240_CHECK_MONITOR
		g_monitor_cmd.a |= 0x08;
		g_monitor_cmd.a &= ~(0x01);
		del_timer_sync(&sii8240->mhl_timer);
		cancel_work_sync(&sii8240->mhl_link_monitor_work);
		sii8240_scm_call(sii8240, _SCM_SVC_OEM, _SCM_OEM_CMD, &g_monitor_cmd, sizeof(g_monitor_cmd), NULL, 0);
		pr_info("%s() g_monitor_cmd.a = %d\n", __func__, g_monitor_cmd.a);

#endif
	}

	return ret;
}

static int sii8240_tmds_active_hdcp(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	ret = set_mute_mode(sii8240, true);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() set_mute_mode fail %d\n", __func__, __LINE__);
		return ret;
	}

	ret = mhl_modify_reg(tpi, 0x1A,
		TMDS_OUTPUT_CONTROL_MASK, TMDS_OUTPUT_CONTROL_POWER_DOWN);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_modify_reg fail %d\n", __func__, __LINE__);
		return ret;
	}
	ret = mhl_modify_reg(tpi, 0x1A,
		TMDS_OUTPUT_CONTROL_MASK, TMDS_OUTPUT_CONTROL_ACTIVE);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_modify_reg fail %d\n", __func__, __LINE__);
		return ret;
	}
	pr_info("sii8240 : %s\n", __func__);
	ret = sii8240_send_avi_infoframe(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() sii8240_send_avi_infoframe fail %d\n", __func__, __LINE__);
		return ret;
	}

	return ret;
}
#endif
static int tmds_control(struct sii8240_data *sii8240, bool tmds_on)
{
	int ret;
#ifdef SFEATURE_HDCP_SUPPORT
	u8 value, value2;
#endif
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	sii8240->tmds_enable = tmds_on;
	pr_info("sii8240 : %s() TMDS ==> %d\n", __func__, tmds_on);

	ret = mhl_modify_reg(hdmi, 0x81, 0x3F, 0x3C);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d mhl_modify_reg failed !\n",
				__func__, __LINE__);
		return ret;
	}
	ret = mhl_modify_reg(hdmi, 0x87, 0x07, 0x03);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d mhl_modify_reg failed !\n",
				__func__, __LINE__);
		return ret;
	}

	switch (tmds_on) {
	case true:
#ifdef SFEATURE_HDCP_SUPPORT
		ret = mhl_read_byte_reg(tpi, 0x1A, &value);
		if (TMDS_OUTPUT_CONTROL_POWER_DOWN & value) {
			pr_info("sii8240: TMDS status is power_down\n");
			ret = sii8240_tmds_active_hdcp(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: %s():%d sii8240_tmds_active_hdcp failed !\n",
						__func__, __LINE__);
				return ret;
			}
		} else {
			ret = mhl_read_byte_reg(tpi, 0x29, &value2);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d mhl_read_byte_reg failed !\n",
						__func__, __LINE__);
				return ret;
			}
			if (LINK_STATUS_NORMAL != (LINK_STATUS_MASK & value2)) {
				ret = sii8240_tmds_active_hdcp(sii8240);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: %s():%d sii8240_tmds_active_hdcp failed !\n",
							__func__, __LINE__);
					return ret;
				}
			} else if (AV_MUTE_MUTED & value) {
				ret = set_mute_mode(sii8240, false);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: %s():%d set_mute_mode failed !\n",
							__func__, __LINE__);
					return ret;
				}
			}
		}
#else
		ret = mhl_modify_reg(tpi, 0x1A,
			AV_MUTE_MASK|TMDS_OUTPUT_CONTROL_MASK,
			AV_MUTE_NORMAL|TMDS_OUTPUT_CONTROL_ACTIVE);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d mhl_modify_reg failed !\n",
					__func__, __LINE__);
			return ret;
		}
		ret = sii8240_send_avi_infoframe(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d sii8240_send_avi_infoframe failed !\n",
					__func__, __LINE__);
			return ret;
		}
		if (unlikely(ret < 0))
			pr_err("[ERROR] %s() send AVIF fail\n", __func__);
#endif
		break;

	case false:
#ifdef SFEATURE_HDCP_SUPPORT
		sii8240_hdcp_on(sii8240, false);
#endif
		sii8240->regs.intr_masks.intr_tpi_mask_value = 0x0;
		ret = mhl_write_byte_reg(tpi, 0x3C, 0x0);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}

		pr_info("TMDS_OUTPUT_CONTROL_POWER_DOWN |AV_MUTE_NORMAL\n");
		ret = mhl_modify_reg(tpi, 0x1A,
			TMDS_OUTPUT_CONTROL_MASK | AV_MUTE_MASK,
			TMDS_OUTPUT_CONTROL_POWER_DOWN | AV_MUTE_MASK);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		break;

	default:
		pr_err("[ERROR] %s() unknown value\n", __func__);
		break;
	}
	return ret;
}

static int set_hdmi_mode(struct sii8240_data *sii8240, bool hdmi_mode)
{
	int ret;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	ret = mhl_modify_reg(hdmi, 0xA1,
			BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_MASK,
			BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_SW_CTRL);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			 __func__, __LINE__);
		return ret;
	}

	if (hdmi_mode) {
		ret = mhl_modify_reg(hdmi, 0xA1,
			BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_MASK,
			BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_HDMI);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}

		ret = mhl_modify_reg(tpi, 0x1A, TMDS_OUTPUT_MODE_MASK,
			TMDS_OUTPUT_MODE_HDMI);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		ret = mhl_write_byte_reg(hdmi, 0x90, 0xF5);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}

		ret = mhl_write_byte_reg(hdmi, 0x91, 0x06);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		ret = mhl_modify_reg(hdmi, 0xA3,
			BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_MASK,
			BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_DISABLE);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
	} else{
		ret = mhl_modify_reg(hdmi, 0xA1,
			BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_MASK,
			BIT_REG_RX_HDMI_CTRL0_hdmi_mode_sw_value_DVI);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}

		ret = mhl_modify_reg(tpi, 0x1A, TMDS_OUTPUT_MODE_MASK,
			TMDS_OUTPUT_MODE_DVI);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		ret = mhl_write_byte_reg(hdmi, 0x90, 0xFF);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		ret = mhl_write_byte_reg(hdmi, 0x91, 0xFF);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		ret = mhl_modify_reg(hdmi, 0xA3,
		       BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_MASK,
		       BIT_RX_HDMI_CTRL2_USE_AV_MUTE_SUPPORT_DISABLE);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}

	}

	sii8240->hdmi_mode = hdmi_mode;
	pr_info("sii8240:  %s():HDMI mode = %d\n", __func__,  hdmi_mode);

	return ret;
}

static int sii8240_fifo_clear(struct sii8240_data *sii8240)
{
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	u8 data;
	int ret;

	pr_info("%s\n", __func__);

	ret = mhl_read_byte_reg(tmds, 0xf2, &data);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() :%d mhl_read_byte_reg failed!\n", __func__, __LINE__);
		return ret;
	}
	ret = mhl_set_reg(tmds,
		TPI_DISABLE_REG, SW_TPI_EN_MASK);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() :%d mhl_set_reg failed!\n", __func__, __LINE__);
		return ret;
	}
	if (0x20 & data) {
		ret = mhl_write_byte_reg(tmds,
			0xF2, data & ~0x20);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() :%d mhl_write_byte_reg failed!\n", __func__, __LINE__);
			return ret;
		}
		ret = mhl_modify_reg(tmds, 0xF3, 0x0F, 0x09);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() :%d mhl_modify_reg failed!\n", __func__, __LINE__);
			return ret;
		}
	}
	ret = mhl_clear_reg(tmds,
		TPI_DISABLE_REG, SW_TPI_EN_MASK);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() :%d mhl_clear_reg failed!\n", __func__, __LINE__);
		return ret;
	}
	return ret;
}

#ifdef SFEATURE_HDCP_SUPPORT
static int sii8240_hdcp_key_check(struct sii8240_data *sii8240)
{
	u8 aksv[AKSV_SIZE];
	int ret, i, cnt;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	memset(aksv, 0x00, AKSV_SIZE);
	cnt = 0;

	ret = mhl_read_block_reg(tpi, HDCP_KEY, AKSV_SIZE, aksv);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			 __func__, __LINE__);
		return ret;
	}

	pr_info("sii8240: AKSV :0x%x, 0x%x, 0x%x, 0x%x, 0x%x",
		aksv[0], aksv[1], aksv[2], aksv[3], aksv[4]);
	for (i = 0; i < AKSV_SIZE; i++) {
		while (aksv[i] != 0x00) {
			if (aksv[i] & 0x01)
				cnt++;

			aksv[i] >>= 1;
		}
	}
	if (cnt != NUM_OF_ONES_IN_KSV) {
		pr_cont(" -> Illegal AKSV !\n");
		ret = -EINVAL;
	} else
		pr_cont(" ->ok\n");

	return ret;
}

static int sii8240_hdcp_control(struct sii8240_data *sii8240, u8 hdcp_reg)
{
	int ret;
	u8 rd_data = 0, hdcp_query;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	ret = mhl_read_byte_reg(tpi, TPI_HDCP_QUERY_DATA_REG, &hdcp_query);
	if (unlikely(ret < 0)) {
		pr_info("sii8240: HDCP query request fail\n");
		return ret;
	}
	/*HDCP link status changed.
	  *Indicates a status change event in the LinkStatus (0x29) value.*/
	if (hdcp_reg & BIT_TPI_INTR_ST0_HDCP_SECURITY_CHANGE_EVENT) {
		rd_data = hdcp_query & LINK_STATUS_MASK;

		switch (rd_data) {
		case LINK_STATUS_NORMAL:
			pr_info("sii8240: %s():%d LINK_STATUS_NORMAL !!!\n",
					__func__, __LINE__);
			break;

		case LINK_STATUS_LINK_LOST:
			pr_info("sii8240: %s():%d LINK_STATUS_LINK_LOST !!!\n",
				__func__, __LINE__);
			if ((sii8240->hdmi_sink == false) && ((hdcp_query & 0x08) == 0)) {
				ret = set_mute_mode(sii8240, false);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: set_mute_mode on fail\n");
					return ret;
				}
			}
			ret = sii8240_hdcp_on(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: tmds_control on fail\n");
				return ret;
			}
			ret = sii8240_tmds_active_hdcp(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: sii8240_tmds_active_hdcp fail\n");
				return ret;
			}
			break;

		case LINK_STATUS_RENEGOTIATION_REQ:
			pr_info("sii8240: %s():%d LINK_STATUS_RENEGOTIATION_REQ !!!\n",
				__func__, __LINE__);
			sii8240_fifo_clear(sii8240);
			ret = sii8240_hdcp_on(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: sii8240_hdcp_on off fail\n");
				return ret;
			}
			msleep(100);
			ret = sii8240_tmds_active_hdcp(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: set_mute_mode on fail\n");
				return ret;
			}
			break;

		case LINK_STATUS_LINK_SUSPENDED:
			pr_info("sii8240: %s():%d LINK_STATUS_LINK_SUSPENDED !!!\n",
				__func__, __LINE__);

			ret = sii8240_hdcp_on(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: hdcp off fail\n");
				return ret;
			}
			break;
		}
	}

	/*HDCP Authentication status changed.
	*Indicates either that the previous authentication request has completed
	*or that an Ri mismatch has caused authentication to fail.*/
	if (hdcp_reg & BIT_TPI_INTR_ST0_HDCP_AUTH_STATUS_CHANGE_EVENT) {
		rd_data = hdcp_query &
			(EXTENDED_LINK_PROTECTION_MASK |
			LOCAL_LINK_PROTECTION_MASK);

		switch (rd_data) {
		case (EXTENDED_LINK_PROTECTION_NONE |
			LOCAL_LINK_PROTECTION_NONE):
			pr_info("EXTENDED_LINK_PROTECTION_NONE\n");
			ret = sii8240_hdcp_on(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: sii8240_hdcp_on  fail\n");
				return ret;
			}

			ret = sii8240_tmds_active_hdcp(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR]sii8240: sii8240_tmds_active_hdcp fail\n");
				return ret;
			}
			break;

		case LOCAL_LINK_PROTECTION_SECURE:
			pr_info("sii8240: %s():%d LOCAL_LINK_PROTECTION_SECURE\n",
				__func__, __LINE__);
			if (!(HDCP_REPEATER_MASK & hdcp_query)) {
				ret = set_mute_mode(sii8240, false);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR]sii8240: set_mute_mode off fail\n");
					return ret;
				}
#ifdef SII8240_CHECK_MONITOR
				if ((g_monitor_cmd.a & 0x01) == 0x01) {
					/*start checking link status*/
					sii8240_link_monitor_timer((unsigned long) sii8240);
					pr_info("%s():%d sii8240_link_monitor_timer\n",
						__func__, __LINE__);
				} else
					pr_err("[ERROR] : %s monitor state : %d\n",
								__func__, g_monitor_cmd.a);
#endif
			}
			break;

		case (EXTENDED_LINK_PROTECTION_SECURE |
			LOCAL_LINK_PROTECTION_SECURE):
			pr_info("EXTENDED_LINK_PROTECTION_SECURE\n");
			if (HDCP_REPEATER_MASK & hdcp_query) {
				ret = set_mute_mode(sii8240, false);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR]sii8240: set_mute_mode off fail\n");
					return ret;
				}
#ifdef SII8240_CHECK_MONITOR
				if ((g_monitor_cmd.a & 0x01) == 0x01) {
					/*start checking link status*/
					sii8240_link_monitor_timer((unsigned long) sii8240);
					pr_info("%s():%d sii8240_link_monitor_timer\n",
						__func__, __LINE__);
				} else
					pr_err("[ERROR] : %s monitor state : %d\n",
								__func__, g_monitor_cmd.a);
#endif
			}
			break;

		default:
			pr_info("sii8240: %s():%d default !!!\n",
			__func__, __LINE__);
			ret = sii8240_hdcp_on(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: sii8240_hdcp_on fail\n");
				return ret;
			}

			ret = sii8240_tmds_active_hdcp(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s(%d) sii8240: tmds_control off fail\n", __func__, __LINE__);
				return ret;
			}
			break;
		}
	}
	/*Read BKSV Done.
	*Various bits in 0x29 contain information acquired from BCAPs.*/
	if (hdcp_reg & BIT_TPI_INTR_ST0_BKSV_DONE) {
		pr_info("sii8240: %s():%d BIT_TPI_INTR_ST0_BKSV_DONE\n",
			__func__, __LINE__);
		ret = set_hdmi_mode(sii8240, sii8240->hdmi_sink);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d set_hdmi_mode fail\n",
					__func__, __LINE__);
			return ret;
		}
		/*HDCP on*/
		if (hdcp_query & PROTECTION_TYPE_MASK) {
			ret = sii8240_hdcp_on(sii8240, true);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s(%d):sii8240_hdcp_on is fail\n",
				__func__, __LINE__);
				ret = sii8240_hdcp_on(sii8240, false);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] %s(%d):sii8240_hdcp_on is fail\n",
						__func__, __LINE__);
					return ret;
				}
				ret = sii8240_tmds_active_hdcp(sii8240);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: tmds_control off fail\n");
					return ret;
				}
			}
		}
	}
	if (hdcp_reg & BIT_TPI_INTR_ST0_BKSV_ERR) {
		pr_info("sii8240: %s():bksv err\n", __func__);
		ret = sii8240_hdcp_on(sii8240, false);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s(%d):sii8240_hdcp_on is fail\n",
				__func__, __LINE__);
			return ret;
		}
		ret = sii8240_tmds_active_hdcp(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: tmds_control off fail\n");
			return ret;
		}
	}
	return 0;
}
#endif
static void sii8240_check_hdmi_mode(struct sii8240_data *sii8240, int exts)
{
	u8 datablock;
	u8 *edid;
	int i, ieee_reg;

	/* 3 = VSD */
	datablock = 3;
	i = 0x4;
	edid = sii8240->edid + (exts * EDID_LENGTH);
	for (; i < edid[2]; i += ((edid[i] & 0x1f) + 1)) {
		/* Find vendor specific block */
		if ((edid[i] >> 5) == datablock) {
			ieee_reg = edid[i + 1] | (edid[i + 2] << 8) |
				edid[i + 3] << 16;
			if (ieee_reg == 0x000c03) {
				pr_info("sii9240: hdmi_sink\n");
				sii8240->hdmi_sink = true;
			}
			break;
		}
	}
}

static int sii8240_read_edid_block(struct sii8240_data *sii8240, u8 edid_ext)
{
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	int ret;
	int i = 0;
	u8 data;
	u16 temp = edid_ext << 7;

	ret = mhl_set_reg(hdmi, EDID_STATUS_HW_ASSIST_REG, HW_EDID_DONE);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, EDID_CTRL_REG,
			EDID_MODE_EN | EDID_FIFO_ADDR_AUTO);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return ret;
	}

	if (!edid_ext) {	/* Block-0 */
		ret = mhl_write_byte_reg(hdmi, EDID_PAGE_HW_ASSIST_REG,
			HW_READ_EDID_BLOCK_0);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
	} else {		/* other EDID-extension blocks */
		ret = mhl_write_byte_reg(hdmi, EDID_BLOCK_ADDR_HW_ASSIST_REG,
			(1<<(edid_ext-1)));
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
	}


	/* TODO: use some completion mechanism or wait_for_completion_timeout
	 * APIs instead of this loop */
	do {
		ret = mhl_read_byte_reg(hdmi, EDID_STATUS_HW_ASSIST_REG,
			&data);
		if (unlikely(ret < 0)) {
			pr_info("sii8240:edid reg read failed!\n");
			return ret;
		}
		if (data & HW_EDID_DONE) {
			ret = mhl_write_byte_reg(hdmi,
						EDID_STATUS_HW_ASSIST_REG,
						HW_EDID_DONE);
			if (unlikely(ret < 0)) {
				pr_info("sii8240: edid done failed!\n");
				return ret;
			}
			break;
		}
		if (data & HW_EDID_ERROR) {
			ret = mhl_write_byte_reg(hdmi,
						EDID_STATUS_HW_ASSIST_REG,
						HW_EDID_ERROR);
			if (unlikely(ret < 0)) {
				pr_info("sii8240: edid read error!\n");
				return ret;
			}

			sii8240_fifo_clear(sii8240);

			ret = mhl_write_byte_reg(hdmi, EDID_PAGE_HW_ASSIST_REG,
						HW_READ_EDID_BLOCK_0);
			if (unlikely(ret < 0)) {
				pr_info("sii8240: edid block 0 failed !\n");
				return ret;
			}
		}
		usleep_range(1000, 2000);
		i++;

	} while (i < 100);

	if (i == 100) {
		pr_info("sii8240:%s():%d EDID READ timeout\n",
							__func__, __LINE__);
		return -ETIMEDOUT;
	}

	/* CHECK: 0,(1<<7),0,(1<<7) values being used in reference driver */
	ret = mhl_write_byte_reg(hdmi, EDID_FIFO_ADDR_REG, (temp & 0xFF));
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return ret;
	}

	/* TODO 1: We can optimize this loop using loop unrolling techniques */
	/* TODO 2: In one of the reference driver, block read is being used;
		   need to investiage on this */
	/* SMBus allows at most 32 bytes, so read by 32 bytes 4 times. */
	for (i = 0; i < EDID_LENGTH/I2C_SMBUS_BLOCK_MAX; i++) {
		ret = mhl_read_block_reg(hdmi, EDID_FIFO_RD_DATA_REG,
				I2C_SMBUS_BLOCK_MAX,
				(&sii8240->edid[i*I2C_SMBUS_BLOCK_MAX +
				 edid_ext*EDID_LENGTH]));
		if (unlikely(ret < 0)) {
			pr_err("failed to read EDID_FIFO_RD_DATA_REG\n");
			return ret;
		}
	}
	return ret;
}

u8 sii8240_mhl_get_version(void)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);
	return sii8240->regs.peer_devcap[MHL_DEVCAP_MHL_VERSION];
}

u8 sii8240_support_packedpixel(void)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);
	return (sii8240->regs.peer_devcap[MHL_DEVCAP_VID_LINK_MODE] >> 3) & 0x01;
}

u8 *sii8240_get_mhl_edid(void)
{
	pr_info("sii8240->edid : 0x%p\n", g_sii8240->edid);
	return g_sii8240->edid;
}

static int sii8240_read_edid(struct sii8240_data *sii8240)
{
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	int ret;
	int i;
	u8 edid_exts;

	sii8240->hdmi_sink = false;
	memset(sii8240->edid, 0, sizeof(sii8240->edid));
	/* Read EDID block-0 */
	ret = sii8240_read_edid_block(sii8240, 0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d sii8240_read_edid_block\n",
			__func__, __LINE__);
		goto err_exit;
	}

	edid_exts = sii8240->edid[0x7e];	/* no. of edid extensions */

	/* boundary check for edid_exist: especially when mhl urgents
	   for disconnection, edid_exts get wrong value, in this case
	   reset edid_exts=1(default) to avoid kernel panic
	   by exceding sii8240->edid[0x7e] array boundary */
	if (edid_exts >= (EDID_MAX_LENGTH/EDID_LENGTH)) {
		pr_err("[ERROR] sii8240: edid_exts = %d is wrong\n", edid_exts);
		sii8240->edid[0x7e] = 0x00;
		edid_exts = 0;
	}

	if (!edid_exts) {
		ret = mhl_write_byte_reg(hdmi, EDID_BLOCK_ADDR_HW_ASSIST_REG,
								(1<<0));
		if (ret < 0)
			return ret;
		goto edid_populated;
	}

	for (i = 1; i <= edid_exts; i++) {
		ret = sii8240_read_edid_block(sii8240, i);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d sii8240_read_edid_block\n",
				__func__, __LINE__);
			goto err_exit;
		}
		/* check hdmi mode if CEA BLOCK(ext edid[0] == 0x02) */
		if (sii8240->edid[i * EDID_LENGTH] == 0x02)
			sii8240_check_hdmi_mode(sii8240, i);
	}
edid_populated:
	print_hex_dump(KERN_INFO, "EDID = ",
		DUMP_PREFIX_OFFSET, 16, 1,
			sii8240->edid, EDID_LENGTH * (1 + edid_exts), false);

	ret = mhl_write_byte_reg(hdmi, EDID_FIFO_ADDR_REG, 0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d !\n",
			__func__, __LINE__);
		goto err_exit;
	}

#if 0 /* this is for AP side edid reading through DDC */
	/* Block operations can handle only 32 bytes at a time */
	for (i = 0; i < (edid_exts+1)*(EDID_LENGTH/I2C_SMBUS_BLOCK_MAX); i++) {
		ret = mhl_write_block_reg(hdmi, EDID_FIFO_WR_DATA_REG,
				I2C_SMBUS_BLOCK_MAX,
				&sii8240->edid[i*I2C_SMBUS_BLOCK_MAX]);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: edid write block error\n");
			break;
		}
	}
#endif
	ret = mhl_write_byte_reg(hdmi, EDID_CTRL_REG, EDID_PRIME_VALID |
					EDID_FIFO_ADDR_AUTO | EDID_MODE_EN);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d mhl_write_byte_reg\n",
			__func__, __LINE__);
		goto err_exit;
	}

	ret = mhl_set_reg(sii8240->pdata->disc_client, POWER_CTRL_REG, PCLK_EN);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d mhl_set_reg\n",
			__func__, __LINE__);
		goto err_exit;
	}
	pr_info("sii8240: edid read and stored successfully\n");
	return ret;

err_exit:
	pr_err("[ERROR] err_exit: hdmi_sink set to false\n");
	sii8240->hdmi_sink = false;
	return ret;
}
static int mhl_hpd_control_low(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;

	ret = mhl_modify_reg(tmds, UPSTRM_HPD_CTRL_REG,
		BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK |
		BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK,
		BIT_HPD_CTRL_HPD_OUT_OVR_VAL_OFF |
		BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON);
	if (unlikely(ret < 0)) {
		pr_warn("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}
	sii8240->hpd_status = false;

	return ret;
}

static int force_usb_id_switch_open(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *disc = sii8240->pdata->disc_client;

	ret = mhl_modify_reg(disc, 0x10, 0x1, 0x0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d !\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_modify_reg(disc, 0x15,
		BIT_DC6_USB_OVERRIDE_MASK, BIT_DC6_USB_OVERRIDE_ON);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d !\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(disc, 0x12, BIT_DC3_COMM_IMME_ON |
					BIT_DC3_FORCE_MHL_OFF |
					BIT_DC3_DISC_SIMODE_OFF	|
					BIT_DC3_FORCE_USB_OFF |
					BIT_DC3_USB_EN_OFF |
					BIT_DC3_DLYTRG_SEL_064ms);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_hpd_control_low(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			__func__, __LINE__);
		return ret;
	}

	return ret;
}

static int release_usb_id_switch_open(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *disc = sii8240->pdata->disc_client;

	msleep(50);

	ret = mhl_modify_reg(disc, 0x15, 0x40, 0x0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	/*enable discovery*/
	ret = mhl_modify_reg(disc, 0x10, 0x1, 0x1);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	return ret;
}
static int  sii8240_set_interrupt(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *disc = sii8240->pdata->disc_client;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;

	ret = mhl_write_byte_reg(tmds, 0x75,
		sii8240->regs.intr_masks.intr1_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x76,
		sii8240->regs.intr_masks.intr2_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x77,
		sii8240->regs.intr_masks.intr3_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(disc, DISC_INTR_ENABLE_REG,
		sii8240->regs.intr_masks.intr4_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x78,
		sii8240->regs.intr_masks.intr5_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(cbus, CBUS_MSC_INTR_ENABLE_REG,
		sii8240->regs.intr_masks.intr_cbus0_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(cbus, CBUS_MSC_ERROR_INTR_ENABLE_REG,
		sii8240->regs.intr_masks.intr_cbus1_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x7D,
		sii8240->regs.intr_masks.intr7_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x7E,
		sii8240->regs.intr_masks.intr8_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tpi, 0x3C,
		sii8240->regs.intr_masks.intr_tpi_mask_value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	return ret;
}

/* Functions for Switching Power States, Some boards use these
 * functions,hence adding it here */
static int switch_to_d3(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	struct i2c_client *disc = sii8240->pdata->disc_client;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	memset(&sii8240->regs.intr_masks, 0, sizeof(sii8240->regs.intr_masks));
	sii8240->regs.intr_masks.intr4_mask_value = BIT_INTR4_RGND_DETECTION;
	sii8240->regs.link_mode = MHL_STATUS_CLK_NORMAL;
	ret = sii8240_set_interrupt(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() sii8240_set_interrupt\n", __func__);
		return ret;
	}
	ret = mhl_hpd_control_low(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, MHLTX_TERM_CTRL_REG, 0xD0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	/*Clear all interrupt*/
	ret = mhl_write_byte_reg(disc, 0x21, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(tmds, 0x71, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(tmds, 0x72, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(tmds, 0x73, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(tmds, 0x74, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x7B, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(tmds, 0x7C, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(tmds, 0xE0, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(cbus, 0x8C, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x8E, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x92, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x94, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x96, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x98, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x9A, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_write_byte_reg(cbus, 0x9C, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tpi, 0x3D, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}

	msleep(50);

	ret = mhl_modify_reg(disc, DISC_CTRL1_REG, 1<<0x0, 0x1);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}
	ret = mhl_modify_reg(disc, 0x01, 1<<0x0, 0x0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	sii8240->state = STATE_MHL_READY_RGND_DETECT;
	pr_info("sii8240: D3: Power saving mode\n");
	if (!sii8240->irq_enabled) {
		enable_irq(sii8240->irq);
		sii8240->irq_enabled = true;
		pr_info("sii8240: interrupt enabled\n");
	}

	return 0;
}

/* toggle hpd line low for 100ms */
static int sii8240_toggle_hpd(struct i2c_client *client)
{
	int ret;
	ret = mhl_set_reg(client, UPSTRM_HPD_CTRL_REG, HPD_OVERRIDE_EN);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}
	ret = mhl_clear_reg(client, UPSTRM_HPD_CTRL_REG, HPD_OUT_OVERRIDE_VAL);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}
	msleep(100);
	ret = mhl_set_reg(client, UPSTRM_HPD_CTRL_REG, HPD_OUT_OVERRIDE_VAL);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}
	ret = mhl_clear_reg(client, UPSTRM_HPD_CTRL_REG, HPD_OVERRIDE_EN);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}
	return ret;
}

static int sii8240_host_devcap_init(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;

	u8 devcap[] = { DEV_STATE, DEV_MHL_VERSION, DEV_CAT_SOURCE_NO_PWR,
			DEV_ADOPTER_ID_H, DEV_ADOPTER_ID_L, DEV_VID_LINK_MODE,
			DEV_AUDIO_LINK_MODE, DEV_VIDEO_TYPE, DEV_LOGICAL_DEV,
			DEV_BANDWIDTH, DEV_FEATURE_FLAG, DEV_DEVICE_ID_H,
			DEV_DEVICE_ID_L, DEV_SCRATCHPAD_SIZE,
			DEV_INT_STATUS_SIZE, DEV_RESERVED };

	for (ret = 0; ret < DEVCAP_COUNT_MAX; ret++)
		sii8240->regs.host_devcap[ret] = devcap[ret];

	ret = mhl_write_block_reg(cbus, MHL_DEVCAP_DEVSTATE,
					DEVCAP_COUNT_MAX, devcap);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	return 0;
}

/* Do we really need to do this? */
static int sii8240_cbus_init(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;

	ret = mhl_write_byte_reg(cbus, CBUS_MHL_INTR_REG_0_MASK, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(cbus, CBUS_MHL_INTR_REG_1_MASK, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(cbus, CBUS_MHL_INTR_REG_2_MASK, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(cbus, CBUS_MHL_INTR_REG_3_MASK, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	return ret;
}

static int sii8240_init_regs(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *disc = sii8240->pdata->disc_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;

	pr_info("sii8240_init_regs\n");
	memset(sii8240->regs.peer_devcap, 0x0,
			sizeof(sii8240->regs.peer_devcap));

	ret = mhl_modify_reg(disc, INT_CTRL_REG, 0x06, 0x00);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n", __func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(disc, POWER_CTRL_REG, POWER_TO_D0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
		return ret;
	}
	if (sii8240->state == STATE_MHL_CONNECTED) {
		ret = mhl_modify_reg(disc, DISC_CTRL1_REG, (1<<1), 0x00);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
					__func__, __LINE__);
			return ret;
		}
	}
	ret = mhl_write_byte_reg(hdmi, TMDS_CLK_EN_REG, 0x01);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, TMDS_CH_EN_REG, 0x11);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_clear_reg(tmds, TPI_DISABLE_REG, SW_TPI_EN_MASK);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to clear register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tpi, TPI_HDCP_CTRL_REG, 0x00);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_set_reg(tpi, TPI_AV_MUTE_REG, AV_MUTE_MASK);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tpi, 0xBB, 0x76);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tpi, 0x3D, 0xFF);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, 0xA4, 0x00);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, 0x80,
		BIT_MHLTX_CTL1_TX_TERM_MODE_100DIFF |
		BIT_MHLTX_CTL1_DISC_OVRIDE_ON);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_modify_reg(hdmi, 0x82, BIT_MHLTX_CTL3_DAMPING_SEL_MASK,
		sii8240->pdata->damping);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

#if CONFIG_MHL_SWING_LEVEL
	ret = mhl_modify_reg(hdmi, MHLTX_CTL4_REG, BIT_CLK_SWING_CTL_MASK |
			BIT_DATA_SWING_CTL_MASK, sii8240->pdata->swing_level);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}
#else
	ret = mhl_modify_reg(hdmi, 0x83, BIT_CLK_SWING_CTL_MASK |
		BIT_DATA_SWING_CTL_MASK, 0x33);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}
#endif

	ret = mhl_write_byte_reg(cbus, 0xA7, 0x1C);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
			__func__, __LINE__);
		return ret;
	}

	if (sii8240->state == STATE_MHL_CONNECTED) {
		ret = mhl_write_byte_reg(hdmi, MHLTX_TERM_CTRL_REG, 0x10);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}

#if CONFIG_MHL_SWING_LEVEL
		ret = mhl_modify_reg(hdmi,
				MHLTX_CTL4_REG, BIT_CLK_SWING_CTL_MASK |
				BIT_DATA_SWING_CTL_MASK
				, sii8240->pdata->swing_level);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}
#else
		ret = mhl_modify_reg(hdmi,
			MHLTX_CTL4_REG, BIT_CLK_SWING_CTL_MASK |
			BIT_DATA_SWING_CTL_MASK, 0x34);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}
#endif
	}
	ret = mhl_write_byte_reg(hdmi, 0x87, 0x0A);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tmds, 0x85, 0x02);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, 0x00, 0x00);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, 0x4C, 0xD0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(disc, 0x11, 0xA5);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(disc, DISC_CTRL6_REG, 0x11);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(disc, DISC_CTRL9_REG,
				CBUS_LOW_TO_DISCONNECT |
				WAKE_DRVFLT | DISC_PULSE_PROCEED);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	if (sii8240->state == STATE_MHL_CONNECTED) {
		ret = mhl_write_byte_reg(disc, DISC_CTRL1_REG, 0x27);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}
	} else {
		ret = mhl_write_byte_reg(disc, DISC_CTRL1_REG, 0x26);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}
	}


	ret = mhl_write_byte_reg(disc, DISC_CTRL3_REG, 0x86);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	/*This code is for disable RGND open interrupt */
	/*
	ret = mhl_write_byte_reg(disc, 0x13, 0xAC);
	if (unlikely(ret < 0))
		return ret;
	*/

	ret = mhl_write_byte_reg(hdmi, TPI_PACKET_FILTER_REG,
					DROP_GCP_PKT | DROP_AVI_PKT |
					DROP_MPEG_PKT | DROP_SPIF_PKT |
					DROP_CEA_CP_PKT | DROP_CEA_GAMUT_PKT);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	if (sii8240->state == STATE_DISCONNECTED) {
		ret = mhl_modify_reg(disc, POWER_CTRL_REG,
			BIT_DPD_PDIDCK_MASK, BIT_DPD_PDIDCK_POWER_DOWN);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}
	}
	if (sii8240->state == STATE_MHL_CONNECTED) {
		ret = mhl_modify_reg(disc, POWER_CTRL_REG,
				BIT_DPD_PDIDCK_MASK, BIT_DPD_PDIDCK_MASK);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to write register\n",
				__func__, __LINE__);
			return ret;
		}
	}
	/* In reference driver,HPD_OVERRIDE is enabled and HPD is set to low.
	 * Here, override of hpd is disabled(hence, hpd will propagate from
	 * downstream to upstream, no manual intervention).
	 */
	ret = mhl_modify_reg(tmds, UPSTRM_HPD_CTRL_REG, (1<<6), 0x0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

	ret = mhl_hpd_control_low(sii8240);
	if (sii8240->state == STATE_MHL_CONNECTED) {
		ret = mhl_modify_reg(hdmi, 0xAC,
		 BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_MASK |
		 BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_MASK,
		 BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_W_AVI_EN_CLEAR |
		 BIT_HDMI_CLR_BUFFER_RX_HDMI_VSI_CLR_EN_CLEAR);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
				__func__, __LINE__);
			return ret;
		}
	}
	ret = mhl_write_byte_reg(disc, 0x00, 0x84);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}


	ret = mhl_write_byte_reg(tmds, DCTL_REG, TRANSCODE_OFF |
				TCLK_PHASE_INVERTED);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
	ret =  mhl_modify_reg(tmds, 0x80, BIT_TMDS_CCTRL_CKDT_EN,
					BIT_TMDS_CCTRL_CKDT_EN);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d Fail to set register\n",
			__func__, __LINE__);
		return ret;
	}

#endif
	ret = sii8240_cbus_init(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d sii8240_cbus_init\n",
			__func__, __LINE__);
		return ret;
	}
	ret = sii8240_host_devcap_init(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d sii8240_host_devcap_init\n",
			__func__, __LINE__);
		return ret;
	}


#ifdef SFEATURE_HDCP_SUPPORT
	ret = sii8240_hdcp_key_check(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240_hdcp_key_check failed !\n");
		return ret;
	}
#endif/*SFEATURE_HDCP_SUPPORT*/

	return ret;
}

/* Must call with sii8240->lock held */
static int sii8240_msc_req_locked(struct sii8240_data *sii8240, u8 req_type,
				  u8 offset, u8 first_data, u8 second_data)
{
	int ret = 1;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;
	bool write_offset;
	bool write_first_data;
	bool write_second_data;

	mutex_unlock(&sii8240->lock);
	mutex_lock(&sii8240->msc_lock);

	write_offset = req_type & (START_READ_DEVCAP |
			START_WRITE_STAT_SET_INT | START_WRITE_BURST);
	write_first_data = req_type &
		(START_WRITE_STAT_SET_INT | START_MSC_MSG);
	write_second_data = req_type & START_MSC_MSG;

	pr_info("%s() SEND:offset = 0x%x\n", __func__, offset);
	init_completion(&sii8240->cbus_complete);

	if (write_offset) {
		ret = mhl_write_byte_reg(cbus, MSC_CMD_OR_OFFSET_REG, offset);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			goto err_exit;
		}
	}
	if (write_first_data) {
		ret = mhl_write_byte_reg(cbus, MSC_SEND_DATA1_REG, first_data);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			goto err_exit;
		}
	}
	if (write_second_data) {
		ret = mhl_write_byte_reg(cbus, MSC_SEND_DATA2_REG, second_data);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			goto err_exit;
		}
	}


	ret = mhl_write_byte_reg(cbus, CBUS_MSC_CMD_START_REG, req_type);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		goto err_exit;
	}

	if (!completion_done(&sii8240->cbus_complete)) {
		ret = wait_for_completion_timeout(&sii8240->cbus_complete,
				msecs_to_jiffies(2000));
		if (ret == 0)
			pr_warn("[WARN] sii8240: %s() timeout. type:0x%X, offset:0x%X\n",
					__func__, req_type, offset);
		ret = ret ? 0 : -EIO;
	}
err_exit:
	mutex_unlock(&sii8240->msc_lock);
	mutex_lock(&sii8240->lock);
	return ret;
}

/* Must call with sii8240->lock held */
static int sii8240_devcap_read_locked(struct sii8240_data *sii8240, u8 offset)
{
	int ret;
	u8 val;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;

	if (offset > 0xf)
		return -EINVAL;
	ret = sii8240_msc_req_locked(sii8240, START_READ_DEVCAP, offset, 0, 0);
	if (unlikely(ret < 0))
		pr_warn("[WARN] msc req locked error\n");

	ret = mhl_read_byte_reg(cbus, MSC_RCVD_DATA1_REG, &val);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] msc rcvd data reg read failing\n");
		return ret;
	}
	return val;
}
static int sii8240_queue_devcap_read_locked(struct sii8240_data *sii8240,
		u8 offset)
{
	struct cbus_data *cbus_cmd;
	int ret = 0;

	cbus_cmd = kzalloc(sizeof(struct cbus_data), GFP_KERNEL);
	if (!cbus_cmd) {
		pr_err("[ERROR] sii8240: failed to allocate msc data\n");
		return -ENOMEM;
	}
	cbus_cmd->cmd = READ_DEVCAP;
	cbus_cmd->offset = offset;
	cbus_cmd->use_completion = true;
	init_completion(&cbus_cmd->complete);
	list_add_tail(&cbus_cmd->list, &sii8240->cbus_data_list);

	queue_work(sii8240->cbus_cmd_wqs, &sii8240->cbus_work);
	mutex_unlock(&sii8240->lock);
	ret = wait_for_completion_timeout(&cbus_cmd->complete,
					msecs_to_jiffies(2500));
	mutex_lock(&sii8240->lock);
	if (ret == 0)
		pr_warn("sii8240: read devcap:0x%X time out !!\n", offset);
	else
		kfree(cbus_cmd);

	return ret;
}
static int sii8240_queue_cbus_cmd_locked(struct sii8240_data *sii8240,
					u8 command, u8 offset, u8 data)
{
	struct cbus_data *cbus_cmd;

	cbus_cmd = kzalloc(sizeof(struct cbus_data), GFP_KERNEL);
	if (!cbus_cmd) {
		pr_err("[ERROR] sii8240: failed to allocate msc data\n");
		return -ENOMEM;
	}
	cbus_cmd->cmd = command;
	cbus_cmd->offset = offset;
	cbus_cmd->data = data;	/*modified for 3.1.1.13*/
	cbus_cmd->use_completion = false;
	list_add_tail(&cbus_cmd->list, &sii8240->cbus_data_list);
	queue_work(sii8240->cbus_cmd_wqs, &sii8240->cbus_work);

	return 0;
}
bool is_key_supported(struct sii8240_data *sii8240, int keyindex)
{
	u8 log_dev = DEV_LOGICAL_DEV;

	if (sii8240_rcp_keymap[keyindex].key_code != KEY_UNKNOWN &&
		sii8240_rcp_keymap[keyindex].key_code != KEY_RESERVED &&
		(sii8240_rcp_keymap[keyindex].log_dev_type & log_dev))
		return true;
	 else
		return false;
}

static int sii8240_register_input_device(struct sii8240_data *sii8240)
{
	struct input_dev *input;
	int ret;
	u8 i;

	input = input_allocate_device();
	if (!input) {
		pr_err("[ERROR] sii8240: failed to allocate input device\n");
		return -ENOMEM;
	}
	set_bit(EV_KEY, input->evbit);
	for (i = 0; i < SII8240_RCP_NUM_KEYS; i++)
		sii8240->keycode[i] = sii8240_rcp_keymap[i].key_code;

	input->keycode = sii8240->keycode;
	input->keycodemax = SII8240_RCP_NUM_KEYS;
	input->keycodesize = sizeof(sii8240->keycode[0]);
	for (i = 0; i < SII8240_RCP_NUM_KEYS; i++) {
		if (is_key_supported(sii8240, i))
			set_bit(sii8240->keycode[i], input->keybit);
	}

	input->name = "sii8240_rcp";
	input->id.bustype = BUS_I2C;
	input_set_drvdata(input, sii8240);

	dev_info(&sii8240->pdata->tmds_client->dev,
		"sii8240: registering input device\n");
	ret = input_register_device(input);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: failed to register input device\n");
		input_free_device(input);
		return ret;
	}

	mutex_lock(&sii8240->input_lock);
	sii8240->input_dev = input;
	mutex_unlock(&sii8240->input_lock);

	return 0;
}
static void rcp_key_report(struct sii8240_data *sii8240, u16 key)
{
	pr_info("sii8240: report rcp key: %d\n", key);
	mutex_lock(&sii8240->input_lock);
	if (sii8240->input_dev) {
		input_report_key(sii8240->input_dev, key, 1);
		input_report_key(sii8240->input_dev, key, 0);
		input_sync(sii8240->input_dev);
	}
	mutex_unlock(&sii8240->input_lock);
}

static void cbus_process_rcp_key_locked(struct sii8240_data *sii8240, u8 key)
{
	if (key == 0x7E) {
		pr_info("sii8240 : MHL switch event sent : 1\n");
		switch_set_state(&sii8240->mhl_event_switch, 1);
	}

	if (key < SII8240_RCP_NUM_KEYS) {
		if (is_key_supported(sii8240, key)) {
			/* Report the key */
			rcp_key_report(sii8240, sii8240->keycode[key]);
			/* Send the RCP ack */
			sii8240_msc_req_locked(sii8240, START_MSC_MSG, 0, MSG_RCPK, key);
		} else {
			/* Send a RCPE(RCP Error Message) to Peer followed by RCPK with
			 * old key-code so that initiator(TV) can recognize
			 * failed key code */
			sii8240_msc_req_locked(sii8240, START_MSC_MSG,
					0, MSG_RCPE, RCPE_KEY_INVALID);
		}
	} else {
		/* Input key value is release key
		* Send the RCP ack */
		sii8240_msc_req_locked(sii8240, START_MSC_MSG, 0, MSG_RCPK, key);
	}
}

static void cbus_process_rap_key_locked(struct sii8240_data *sii8240, u8 key)
{
	int ret;
	u8 err = RAPK_NO_ERROR;

	switch (key) {
	case RAP_POLL:
		/* no action, just sent to elicit an ACK */
		break;
	case RAP_CONTENT_ON:
		/*TODO:A source shall not enable its TMDS unless it has received
		 * SET_HPD,sees active RxSense(RSEN) and sees PATH_EN{Sink} = 1
		 */
		ret = tmds_control(sii8240, true);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d tmds_control failed!\n",
					__func__, __LINE__);
			return;
		}
		break;
	case RAP_CONTENT_OFF:
		/*TODO: With MHL 1.2 Specs,For a Source, CONTENT_OFF does not
		 * necessarily means that TMDS output is disabled */
		ret = tmds_control(sii8240, false);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]sii8240: %s():%d tmds_control failed!\n",
					__func__, __LINE__);
			return;
		}
		break;
	default:
		pr_warn("sii8240: unrecognized RAP code %u\n", key);
		err = RAPK_UNRECOGNIZED;
	}

	ret = sii8240_msc_req_locked(sii8240, START_MSC_MSG, 0, MSG_RAPK, err);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d sii8240_msc_req_locked failed!\n",
				__func__, __LINE__);
		return;
	}
}

static void sii8240_power_down(struct sii8240_data *sii8240)
{
	pr_info("%s()\n", __func__);

	mhl_hpd_control_low(sii8240);

	mutex_lock(&sii8240->lock);
	if (sii8240->irq_enabled) {
		disable_irq_nosync(sii8240->irq);
		sii8240->irq_enabled = false;
		pr_info("sii8240: interrupt disabled\n");
	}
	mutex_unlock(&sii8240->lock);

	sii8240->state = STATE_DISCONNECTED;

	cancel_work_sync(&sii8240->cbus_work);
	cancel_work_sync(&sii8240->redetect_work);
	cancel_work_sync(&sii8240->avi_control_work);
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
	del_timer_sync(&sii8240->avi_check_timer);
#endif

	if (sii8240->pdata->power)
		sii8240->pdata->power(0);
}

static int cbus_handle_write_state_locked(struct sii8240_data *sii8240,
							u8 offset, u8 data)
{
	int ret;

	ret = sii8240_msc_req_locked
		(sii8240, START_WRITE_STAT_SET_INT, offset, data, 0);
	if (unlikely(ret < 0))
		return ret;

	if (offset == CBUS_MHL_STATUS_OFFSET_0 &&
		data == MHL_STATUS_DCAP_READY) {
		/* notify the peer by updating the status register too */
		sii8240_msc_req_locked(sii8240, START_WRITE_STAT_SET_INT,
					CBUS_MHL_INTR_REG_0,
					MHL_INT_DCAP_CHG, 0);
	}
	return ret;
}

static void sii8240_setup_charging(struct sii8240_data *sii8240)
{
	u8 plim, dev_cat;
	u16 adopter_id;
	u8 *peer_devcap = sii8240->regs.peer_devcap;

	if ((peer_devcap[MHL_DEVCAP_MHL_VERSION] & 0xF0) >= 0x20) {
		dev_cat = peer_devcap[MHL_DEVCAP_DEV_CAT];
		pr_info("sii8240: DEV_CAT 0x%x\n", dev_cat);
		if (((dev_cat >> 4) & 0x1) == 1) {
			plim = ((dev_cat >> 5) & 0x3);
			pr_info("sii8240 : PLIM 0x%x\n", plim);
			if (sii8240->pdata->charger_mhl_cb)
				sii8240->pdata->charger_mhl_cb(false, plim);
		}
	} else if ((peer_devcap[MHL_DEVCAP_MHL_VERSION] & 0xF0) == 0x10) {
		adopter_id = peer_devcap[MHL_DEVCAP_ADOPTER_ID_L] |
				peer_devcap[MHL_DEVCAP_ADOPTER_ID_H] << 8;
		pr_info("sii8240: adopter id:%d, reserved:%d\n",
				adopter_id, peer_devcap[MHL_DEVCAP_RESERVED]);

		if (adopter_id == 321 && peer_devcap[MHL_DEVCAP_RESERVED] == 2) {
			if (sii8240->pdata->charger_mhl_cb)
				sii8240->pdata->charger_mhl_cb(false, 0x01);
		}
	} else {
		pr_err("sii8240:%s MHL version error - 0x%X\n", __func__,
				peer_devcap[MHL_DEVCAP_MHL_VERSION]);
	}
}

static void sii8240_msc_event(struct work_struct *work)
{
	int ret = -1;
	struct cbus_data *data, *next;
	struct sii8240_data *sii8240 = container_of(work, struct sii8240_data,
			cbus_work);

	mutex_lock(&sii8240->cbus_lock);
	mutex_lock(&sii8240->lock);

	list_for_each_entry_safe(data, next, &sii8240->cbus_data_list, list) {
		if (sii8240->cbus_abort) {
			pr_warn("sii8240 : abort received. so wait 2secs\n");
			sii8240->cbus_abort = false;
			msleep(2000);
		}
		if (sii8240->state != STATE_DISCONNECTED) {
			switch (data->cmd) {
			case MSC_MSG:
				switch (data->offset) {
				case MSG_RCP:
					pr_info("sii8240: RCP Arrived. KEY CODE:%d\n",
					data->data);
					cbus_process_rcp_key_locked(sii8240, data->data);
					break;
				case MSG_RAP:
					pr_info("sii8240: RAP Arrived\n");
					cbus_process_rap_key_locked(sii8240, data->data);
					break;
				case MSG_RCPK:
					pr_info("sii8240: RCPK Arrived\n");
					break;
				case MSG_RCPE:
					pr_info("sii8240: RCPE Arrived\n");
				break;
				case MSG_RAPK:
					pr_info("sii8240: RAPK Arrived\n");
					break;
				default:
					pr_info("sii8240: MAC error\n");
					break;
				}
				break;

			case READ_DEVCAP:
				pr_debug("sii8240: READ_DEVCAP : 0x%X\n",
								data->offset);
				ret = sii8240_devcap_read_locked(sii8240,
								data->offset);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] error offset%d\n",
								data->offset);
					break;
				}
				sii8240->regs.peer_devcap[data->offset] = ret;

				if (data->use_completion)
					complete(&data->complete);

				if (data->offset == MHL_DEVCAP_DEV_CAT)
					sii8240_setup_charging(sii8240);

				ret =  0;
				break;

			case SET_INT:
				pr_info("sii8240: msc_event: SET_INT\n");
				ret = sii8240_msc_req_locked(sii8240,
						START_WRITE_STAT_SET_INT,
						data->offset, data->data, 0);
				if (unlikely(ret < 0))
					pr_err("[ERROR] sii8240: error set_int req\n");
				break;
			case WRITE_STAT:
				pr_info("sii8240: msc_event: WRITE_STAT\n");
				ret = cbus_handle_write_state_locked(sii8240,
						data->offset, data->data);
				if (unlikely(ret < 0))
					pr_err("[ERROR] sii8240: error write_stat\n");
				break;

			case WRITE_BURST:
				/* TODO: */
				break;

			case GET_STATE:
			case GET_VENDOR_ID:
			case SET_HPD:
			case CLR_HPD:
			case GET_MSC_ERR_CODE:
			case GET_SC3_ERR_CODE:
			case GET_SC1_ERR_CODE:
			case GET_DDC_ERR_CODE:
				ret = sii8240_msc_req_locked(sii8240,
					START_MISC_CMD, data->offset,
					data->data, 0);
				if (unlikely(ret < 0))
					pr_err("[ERROR] sii8240: offset%d,data=%d\n",
					data->offset, data->data);

				break;

			default:
				pr_info("sii8240: invalid msc command\n");
				break;
			}
		}
		list_del(&data->list);
		if (!data->use_completion)
			kfree(data);
	}

	mutex_unlock(&sii8240->lock);
	mutex_unlock(&sii8240->cbus_lock);
}

static int sii8240_get_avi_info(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	ret = mhl_modify_reg(hdmi, US_HDMI_INFO_PKT_CTRL,
		BIT_RX_HDMI_CTRL2_VSI_MON_SEL_MASK,
		BIT_RX_HDMI_CTRL2_VSI_MON_SEL_AVI_INFOFRAME);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return ret;
	}

	ret = mhl_read_block_reg(hdmi,
		0xB8, INFO_BUFFER, sii8240->aviInfoFrame);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return ret;
	}
	print_hex_dump(KERN_ERR, "sii8240: avi_info = ",
			DUMP_PREFIX_NONE, 16, 1,
			sii8240->aviInfoFrame, INFO_BUFFER, false);
	return ret;
}
#ifdef MHL_2X_3D
static int sii8240_get_vsi_info(struct sii8240_data *sii8240)
{
	int ret;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	ret = mhl_modify_reg(hdmi,
		0xA3, BIT_RX_HDMI_CTRL2_VSI_MON_SEL_MASK,
		BIT_RX_HDMI_CTRL2_VSI_MON_SEL_VS_INFOFRAME);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_block_reg(hdmi, 0xB8,
			INFO_BUFFER,
			sii8240->vendorSpecificInfoFrame);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

	return ret;
}

/*Video Format Data in 3D support	*
  * 0x00: burst id_h				*
  * 0x01: burst id_l					*
  * 0x02: check_sum				*
  * 0x03: total_entry				*
  * 0x04: sequence index			*
  * 0x05: number of entry			*
  * 0x06: vdi_0_h					*
  * 0x07: vdi_0_l					*
  * 0x08: vdi_1_h					*
  * 0x09: vdi_1_l					*
  * 0x0A: vdi_2_h					*
  * 0x0B: vdi_2_l					*
  * 0x0C: vdi_3_h					*
  * 0x0D: vdi_3_l					*
  * 0x0E: vdi_4_h					*
  * 0x0F: vdi_4_l					*/
static int sii8240_check_3D_vic(struct sii8240_data *sii8240,
					u8 tot_ent, u8 num_ent, u8 *data)
{
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	int ret = 0;

	if ((sii8240->vic_data.tot_ent - (sii8240->vic_data.num_ent + num_ent))
						>= 0) {
		pr_info("num_ent : %d\n", num_ent);
		memcpy(&sii8240->vic_data.vdi[sii8240->vic_data.num_ent],
			data, (2*num_ent));
		sii8240->vic_data.num_ent += num_ent;
	}

	if (sii8240->vic_data.num_ent == sii8240->vic_data.tot_ent) {
		if ((sii8240->hpd_status == true) &&
		(sii8240->dtd_data.num_ent == sii8240->dtd_data.tot_ent)) {
			/*make HPD be high*/
			ret = mhl_modify_reg(tmds, UPSTRM_HPD_CTRL_REG,
			BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK|
			BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK,
			BIT_HPD_CTRL_HPD_OUT_OVR_VAL_ON|
			BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
				return ret;
			}
		}
		pr_info("HPD high\n");
	}

	return ret;
}

static int sii8240_check_3D_dtd(struct sii8240_data *sii8240,
				u8 tot_ent, u8 num_ent, u8 *data)
{
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	int ret = 0;

	if ((sii8240->dtd_data.tot_ent -
			(sii8240->dtd_data.num_ent + num_ent)) >= 0) {
		memcpy(&sii8240->dtd_data.vdi[sii8240->dtd_data.num_ent],
			data, (2*num_ent));
		sii8240->dtd_data.num_ent += num_ent;
	}

	if (sii8240->dtd_data.num_ent == sii8240->dtd_data.tot_ent) {
		if ((sii8240->hpd_status == true) &&
		(sii8240->vic_data.num_ent == sii8240->vic_data.tot_ent)) {
			/*make HPD be high*/
			ret = mhl_modify_reg(tmds, UPSTRM_HPD_CTRL_REG,
			BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK|
			BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK,
			BIT_HPD_CTRL_HPD_OUT_OVR_VAL_ON|
			BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
				return ret;
			}
		}
		pr_info("HPD high\n");
	}

	return ret;
}
#endif

static int sii8240_get_write_burst(struct sii8240_data *sii8240)
{
	struct i2c_client *cbus = sii8240->pdata->cbus_client;
	int ret;
	u8 scratchpad[16];
#ifdef MHL_2X_3D
	u16 burst_id = 0;
#endif
	memset(scratchpad, 0x00, 16);

	ret = mhl_read_block_reg(cbus, CBUS_MHL_SCRPAD_REG_0, 16, scratchpad);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
		__func__, __LINE__);
		return ret;
	}

#ifdef MHL_2X_3D
	burst_id = scratchpad[B_ID_H];
	burst_id <<= 8;
	burst_id |= scratchpad[B_ID_L];

	switch (burst_id) {
	case MHL_3D_VIC_CODE:
		pr_info("vid code\n");

		/*Initiation of first scratchpad*/
		if (scratchpad[SEQ] == 0x01) {
			sii8240->vic_data.tot_ent = scratchpad[TOT_ENT];
			sii8240->vic_data.num_ent = 0;
		}
		ret = sii8240_check_3D_vic(sii8240, scratchpad[TOT_ENT],
			scratchpad[NUM_ENT], &scratchpad[VDI_0_H]);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		break;
	case MHL_3D_DTD_CODE:
		pr_info("dtd code\n");
		if (scratchpad[SEQ] == 0x01) {
			sii8240->dtd_data.tot_ent = scratchpad[TOT_ENT];
			sii8240->dtd_data.num_ent = 0;
		}
		ret = sii8240_check_3D_dtd(sii8240, scratchpad[TOT_ENT],
			scratchpad[NUM_ENT], &scratchpad[VDI_0_H]);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			return ret;
		}
		break;
	default:
		pr_warn("[WARN] invalid burst_id\n");
		break;
	}
#endif
	return ret;
}
static int sii8240_check_avi_info_checksum(struct sii8240_data *sii8240,
						u8 *data)
{
	u8 checksum = 0;
	u8 i = 0;
	u8 infoframedata[INFO_BUFFER];

	memcpy(infoframedata, data, INFO_BUFFER);

	for (i = 0; i < INFO_BUFFER; i++)
		checksum += infoframedata[i];

	checksum = 0x100 - checksum;
	return checksum;
}

static void sii8240_set_avi_info_checksum_data(struct sii8240_data *sii8240)
{
	u8 checksum;
	u8 i;

	/*these are set by the hardware*/
	checksum = 0x82 + 0x02 + 0x0D;

	for (i = 1; i < SIZE_AVI_INFOFRAME; i++)
		checksum += sii8240->output_avi_data[i];

	checksum = 0x100 - checksum;
	sii8240->output_avi_data[0] = checksum;
}

static int sii8240_check_avi_info(struct sii8240_data *sii8240)
{
	int ret = 0, i;

	for (i = 0 ; i < INFO_BUFFER ; i++) {
		if (sii8240->current_aviInfoFrame[i] !=
			sii8240->aviInfoFrame[i])
			ret = 1;
	}

	if (ret == 1) {
		pr_info("%s() current_aviInfoFrame  -------\n", __func__);
		print_hex_dump(KERN_ERR, "sii8240: avi_info = ",
			DUMP_PREFIX_NONE, 16, 1,
			sii8240->current_aviInfoFrame, INFO_BUFFER, false);
		pr_info("%s() aviInfoFrame  -------\n", __func__);
		print_hex_dump(KERN_ERR, "sii8240: avi_info = ",
			DUMP_PREFIX_NONE, 16, 1,
			sii8240->aviInfoFrame, INFO_BUFFER, false);
	}

	return ret;
}

static void sii8240_set_colorspace
	(struct sii8240_data *sii8240, bool pack_pixel)
{
	int ret = 0;
	u8 colorspace = 0;
	u8 input_range = 0;
	u8 data;

	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;

	if (pack_pixel)
		pr_info("sii8240: %s(): pack_pixel\n", __func__);
	else
		pr_info("sii8240: %s(): none pack_pixel\n", __func__);
	if (pack_pixel)
		/*Video stream is encoded in PackedPixel mode*/
		data = BIT_VID_MODE_m1080p_ENABLE;
	else
		data = BIT_VID_MODE_m1080p_DISABLE;

	ret = mhl_modify_reg(tmds, 0x4A, BIT_VID_MODE_m1080p_MASK, data);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return;
	}

	if (pack_pixel)
		/*The incoming HDMI pixel clock is multiplied
			by 2 for PackedPixel mode*/
		data = BIT_MHLTX_CTL4_MHL_CLK_RATIO_2X;
	else
		data = BIT_MHLTX_CTL4_MHL_CLK_RATIO_3X;
	ret = mhl_modify_reg(hdmi, 0x83,
		BIT_MHLTX_CTL4_MHL_CLK_RATIO_MASK, data);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return;
	}

	if (pack_pixel)
		/*the outgoing MHL link clock is
			configured as PackedPixel clock*/
		data = 0x60;
	else
		data = 0xA0;
	ret = mhl_write_byte_reg(hdmi, 0x85, data);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return;
	}
	/*[INFO_DATA1] 0x00 : nodata
	0x20 : YCbCr 4:2:2
	0x40 : YCbCr 4:4:4
	0x60 ; Future*/
	colorspace = (sii8240->current_aviInfoFrame
					[INFO_CHECKSUM + INFO_DATA1] & 0x60);

	switch (colorspace) {
	case 0x00:
		pr_info("RGB :");
		colorspace = 0x00;
	/*[INFO_DATA3] 0x00 : Default
	0x04 : Limited Range
	0x08 : Full Range
	0x0C : Reserved*/
		input_range = (sii8240->current_aviInfoFrame
					[INFO_CHECKSUM + INFO_DATA3] & 0x0C);
		break;
	case 0x20:
		pr_info("YCBCR 422 :");
		colorspace = 0x02;
		input_range = (sii8240->current_aviInfoFrame
					[INFO_CHECKSUM + INFO_DATA5] & 0xC0);
		break;

	case 0x40:
		pr_info("YCBCR 444 :");
		input_range = (sii8240->current_aviInfoFrame
					[INFO_CHECKSUM + INFO_DATA5] & 0xC0);
		colorspace = 0x01;

		break;
	case 0x60:
		pr_info("FUTURE :");
		colorspace = 0x00;
		input_range = (sii8240->current_aviInfoFrame
					[INFO_CHECKSUM + INFO_DATA5] & 0xC0);
		break;
	}

	/*Input range checking*/
	switch (input_range) {
	case 0x00:
		pr_info("input_range AUTO\n");
		input_range = 0x00;
		break;

	case 0x04:
		pr_info("input_range Limited\n");
		input_range = 0x08;
		break;

	case 0x08:
		pr_info("input_range FULL\n");
		input_range = 0x04;
		break;

	case 0x0C:
		pr_info("input_range Reserved\n");
		input_range = 0x00;
		break;

	default:
		pr_info("input_range default\n");
		input_range = 0x00;
		break;
	}

	if (!sii8240->hdmi_sink) {
		colorspace = 0x00;
		input_range = BIT_TPI_OUTPUT_QUAN_RANGE_LIMITED;
	}

	/*The output video encoding is forced to YCbCr 4:2:2
	by setting the input video format and
	output video format at TPI:0x09 and TPI:0x0A.*/
	data = colorspace|input_range;
	ret = mhl_modify_reg(tpi, 0x09,
		BIT_TPI_INPUT_FORMAT_MASK |
		BIT_TPI_INPUT_QUAN_RANGE_MASK, data);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return;
	}

	if (pack_pixel)
		colorspace = 0x02;
	else if (input_range != 0x04)
		input_range = 0x08;
	data = colorspace|input_range;
	ret = mhl_modify_reg(tpi, 0x0A,
		BIT_TPI_OUTPUT_FORMAT_MASK |
		BIT_TPI_OUTPUT_QUAN_RANGE_MASK, data);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
				__func__, __LINE__);
		return;
	}

	return;
}
static int sii8240_set_mhl_timing(struct sii8240_data *sii8240)
{
	/*set packed pixcel information*/
	memset(sii8240->support_mhl_timing.avi_infoframe,
		0x00, HDMI_VFRMT_MAX);

	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_PATTERN_GEN] = 1;
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_1920x1080p60_16_9] = 1;
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_1920x1080p50_16_9] = 1;
#ifdef MHL_2X_3D
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_1280x720p50_16_9] = 1;
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_1280x720p60_16_9] = 1;
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_1920x1080p24_16_9] = 1;
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_1920x1080p30_16_9] = 1;
#endif
	sii8240->support_mhl_timing.
		avi_infoframe[HDMI_VFRMT_640x480p60_4_3] = 1;
	return 1;
}

static bool sii8240_get_mhl_timing(struct sii8240_data *sii8240, u8 vic)
{
	bool ret = false;

	/*get packed pixcel information*/
	if (sii8240->support_mhl_timing.avi_infoframe[vic] == 1)
		ret = true;

	switch (vic) {
	case HDMI_VFRMT_1280x720p50_16_9:
	case HDMI_VFRMT_1280x720p60_16_9:
	case HDMI_VFRMT_1920x1080p24_16_9:
	case HDMI_VFRMT_1920x1080p30_16_9:
		sii8240->input_3d_format = NON_FRAME_PACKING_3D;
		ret = false;
		break;
	default:
		break;
	}

	return ret;
}
static int sii8240_packed_pixel_avi_info_locked(struct sii8240_data *sii8240)
{
	int ret;
	bool patternGen = false;
	u8 vic;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	pr_info("sii8240_packed_pixel_avi_info_locked\n");
	ret = sii8240_check_avi_info_checksum(sii8240, sii8240->aviInfoFrame);
	if (ret) {
			pr_err("[ERROR] sii8240: avi info checksum is fail\n");
			set_hdmi_mode(sii8240, false);
			return ret;
	}

	/*AVI information was changed newly and checksum is also no problems*/
	ret = set_hdmi_mode(sii8240, true);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d set_hdmi_mode fail\n",
				__func__, __LINE__);
		return ret;
	}
	/* VIC = video format Identification Code
	16 : 1920x1080p 60Hz
	31 : 1920x1080p 50HZ
	0 : no VIC format in CEA document */
	vic = sii8240->aviInfoFrame[INFO_VIC];
	patternGen = sii8240_get_mhl_timing(sii8240, vic);

	if (patternGen) {
		pr_info("sii8240: over 75MHz\n");
		sii8240->regs.link_mode &= ~MHL_STATUS_CLK_MODE_NORMAL;
		sii8240->regs.link_mode |= MHL_STATUS_CLK_MODE_PACKED_PIXEL;
	} else {
		sii8240->regs.link_mode &= ~MHL_STATUS_CLK_MODE_PACKED_PIXEL;
		sii8240->regs.link_mode |= MHL_STATUS_CLK_MODE_NORMAL;
	}

	pr_info("vic value = %d, linkmode = 0x%x\n",
		vic, sii8240->regs.link_mode);
	ret = sii8240_msc_req_locked(sii8240, START_WRITE_STAT_SET_INT
		, CBUS_MHL_STATUS_OFFSET_1, sii8240->regs.link_mode, 0);
	if (ret < 0)
		pr_warn("msc req locked error\n");
	/*Copy avi information to current variable*/
	memcpy(&sii8240->current_aviInfoFrame,
		&sii8240->aviInfoFrame, INFO_BUFFER);
	memcpy(&sii8240->output_avi_data[0],
		&sii8240->aviInfoFrame[INFO_CHECKSUM], SIZE_AVI_INFOFRAME);

	if (patternGen) {
		/*Because of the 16bits carried in PackedPixel mode,
		if the incoming video encoding is RGB of YCbCr 4:4:4,
		the video must be converted to YCbCr 4:2:2*/
		sii8240->output_avi_data[INFO_DATA1] &= ~0x60;
		sii8240->output_avi_data[INFO_DATA1] |= 0x20;
	}
	sii8240_set_avi_info_checksum_data(sii8240);

	/*HDMI input clock is under 75MHz*/
	if (vic <= HDMI_VFRMT_720x480p60_16_9 && vic != 0) {
		pr_info("%s() under 75MHz\n", __func__);
		ret = mhl_write_byte_reg(hdmi, 0x4C, 0xD0);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d mhl_write_byte_reg\n",
					__func__, __LINE__);
			return ret;
		}
	}

	/*HDMI input clock is over 75MHz*/
	if (patternGen) {
		ret = mhl_write_byte_reg(hdmi, 0x4C, 0xD0);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d mhl_write_byte_reg\n",
					__func__, __LINE__);
			return ret;
		}
	}

	sii8240_set_colorspace(sii8240, patternGen);

	return ret;
}

static int sii8240_bypass_avi_info_locked(struct sii8240_data *sii8240)
{
	int ret;
	/*struct i2c_client *tpi = sii8240->pdata->tpi_client; */
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	pr_info("sii8240_bypass_avi_info_locked\n");

	ret = sii8240_check_avi_info_checksum(sii8240, sii8240->aviInfoFrame);
	if (ret) {
			pr_err("[ERROR] sii8240: avi info checksum is fail\n");
			set_hdmi_mode(sii8240, false);
			return ret;
	}

	/*AVI information was changed newly and checksum is also no problems*/

	sii8240->regs.link_mode &= ~MHL_STATUS_CLK_MODE_PACKED_PIXEL;
	sii8240->regs.link_mode |= MHL_STATUS_CLK_MODE_NORMAL;

	ret = sii8240_msc_req_locked(sii8240, START_WRITE_STAT_SET_INT,
		CBUS_MHL_STATUS_OFFSET_1, sii8240->regs.link_mode, 0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d sii8240_msc_req_locked fail\n",
				__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(hdmi, 0x4C, 0xD0);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d mhl_write_byte_reg\n",
				__func__, __LINE__);
		return ret;
	}

	/*bypass avi info*/
	memcpy(&sii8240->current_aviInfoFrame,
		&sii8240->aviInfoFrame, INFO_BUFFER);

	pr_info("%s() change current_aviInfoFrame\n", __func__);
	print_hex_dump(KERN_ERR, "sii8240: avi_info = ",
			DUMP_PREFIX_NONE, 16, 1,
			sii8240->current_aviInfoFrame, INFO_BUFFER, false);

	memcpy(&sii8240->output_avi_data[0],
		&sii8240->aviInfoFrame[INFO_CHECKSUM], SIZE_AVI_INFOFRAME);

	sii8240_set_avi_info_checksum_data(sii8240);
	sii8240_set_colorspace(sii8240, false);

	return ret;
}

static void sii8240_avi_control_thread(struct work_struct *work)
{
	struct sii8240_data *sii8240 = container_of(work, struct sii8240_data,
		avi_control_work);
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
#ifdef SFEATURE_HDCP_SUPPORT
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
#endif
	int ret;
#ifdef MHL_2X_3D
	struct cbus_data *cbus_cmd;
	bool feature_3d = false;
#endif

	mutex_lock(&sii8240->lock);

	if (sii8240->avi_work == true) {
		sii8240->avi_work = false;
	switch (sii8240->avi_cmd) {
	case HPD_HIGH_EVENT:
		pr_info("***HPD high\n");
		/*We will read minimum devcap information*/
		sii8240_queue_devcap_read_locked(sii8240,
					MHL_DEVCAP_MHL_VERSION);
		sii8240_queue_devcap_read_locked(sii8240,
					MHL_DEVCAP_VID_LINK_MODE);
		if (sii8240->cbus_ready) {
			pr_info("sii8240: DCAP_READY and read devcap\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
						MHL_DEVCAP_RESERVED, 0) < 0)
				pr_info("sii8240: MHL_RESERVED read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
						MHL_DEVCAP_ADOPTER_ID_H, 0) < 0)
				pr_info("sii8240: ADOPTER_ID_H read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
						MHL_DEVCAP_ADOPTER_ID_L, 0) < 0)
				pr_info("sii8240: ADOPTER_ID_L read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
						MHL_DEVCAP_DEV_CAT, 0) < 0)
				pr_info("sii8240: DEVCAP_DEV_CAT read fail\n");
		}

		if (!sii8240->hpd_status) {
			pr_err("[ERROR] sii8240: hpd_status false\n");
			goto exit;
		}
		pr_info("sii8240: HPD high - MHL ver=0x%x, linkmode = 0x%x\n",
			sii8240->regs.peer_devcap[MHL_DEVCAP_MHL_VERSION],
			sii8240->regs.peer_devcap[MHL_DEVCAP_VID_LINK_MODE]);

		ret = sii8240_read_edid(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: edid read failed\n");
			goto exit;
		}

		if (sii8240->pdata->hdmi_mhl_ops) {
			struct msm_hdmi_mhl_ops *hdmi_mhl_ops =	sii8240->pdata->hdmi_mhl_ops;
			hdmi_mhl_ops->set_upstream_hpd(sii8240->pdata->hdmi_pdev, 1);
		}

		if (((sii8240->regs.peer_devcap[MHL_DEVCAP_MHL_VERSION] & 0xF0) >= 0x20)
		&& (sii8240->regs.peer_devcap[MHL_DEVCAP_VID_LINK_MODE] &
					 (MHL_DEV_VID_LINK_SUPP_PPIXEL |
					MHL_DEV_VID_LINK_SUPPYCBCR422)) &&
					sii8240->hdmi_sink == true) {
			pr_info("CEA_NEW_AVI MHL RX Ver.2.x\n");
#ifdef MHL_2X_3D
			feature_3d = true;
			/*Request 3D interrupt to sink device.
			 * To do msc command*/
			cbus_cmd = kzalloc(sizeof(struct cbus_data),
								GFP_KERNEL);
			if (!cbus_cmd) {
				pr_err("[ERROR] sii8240: failed to allocate cbus data\n");
				goto exit;
			}
			cbus_cmd->cmd = SET_INT;
			cbus_cmd->offset = CBUS_MHL_INTR_REG_0;
			cbus_cmd->data = MHL_INT_3D_REQ;
			list_add_tail(&cbus_cmd->list,
				&sii8240->cbus_data_list);

			queue_work(sii8240->cbus_cmd_wqs, &sii8240->cbus_work);
#endif

		} else {
			/* TODO: This case MHL 1.0*/
			pr_info("CEA_NEW_AVI MHL RX Ver.1.x, Pixel 60\n");
		}

		if (sii8240->hdmi_sink) {
			sii8240->regs.intr_masks.intr7_mask_value =
				BIT_INTR7_CEA_NO_AVI|BIT_INTR7_CEA_NO_VSI;

		ret = mhl_write_byte_reg(tmds, 0x7D,
				sii8240->regs.intr_masks.intr7_mask_value);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] : %s():%d failed !\n",
					__func__, __LINE__);
			goto exit;
		}

		sii8240->regs.intr_masks.intr8_mask_value =
			BIT_INTR8_CEA_NEW_AVI|BIT_INTR8_CEA_NEW_VSI;
		ret = mhl_write_byte_reg(tmds, 0x7E,
				sii8240->regs.intr_masks.intr8_mask_value);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] : %s():%d failed !\n",
					__func__, __LINE__);
			goto exit;
		}

		sii8240->regs.intr_masks.intr5_mask_value =
			BIT_INTR5_CKDT_CHANGE | BIT_INTR5_SCDT_CHANGE;

		ret = mhl_write_byte_reg(tmds, 0x78,
				sii8240->regs.intr_masks.intr5_mask_value);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR]: %s():%d failed !\n",
					__func__, __LINE__);
			goto exit;
		}
		}
		/*make HPD be high*/
		ret = mhl_modify_reg(tmds, UPSTRM_HPD_CTRL_REG,
				BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK |
				BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK,
				BIT_HPD_CTRL_HPD_OUT_OVR_VAL_ON |
				BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
			goto exit;
		}

		if (sii8240->hdmi_sink == false) {
#ifdef SFEATURE_HDCP_SUPPORT
			sii8240->regs.intr_masks.intr_tpi_mask_value =
			BIT_TPI_INTR_ST0_HDCP_AUTH_STATUS_CHANGE_EVENT |
			BIT_TPI_INTR_ST0_HDCP_VPRIME_VALUE_READY_EVENT |
			BIT_TPI_INTR_ST0_HDCP_SECURITY_CHANGE_EVENT |
			BIT_TPI_INTR_ST0_BKSV_DONE |
			BIT_TPI_INTR_ST0_BKSV_ERR;

			ret = mhl_write_byte_reg(tpi, 0x3C,
				sii8240->regs.intr_masks.intr_tpi_mask_value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
				goto exit;
			}
#endif
			ret = set_hdmi_mode(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
				goto exit;
			}
			ret = tmds_control(sii8240, true);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed !\n",
							__func__, __LINE__);
				goto exit;
			}
		}
		break;

	case CEA_NEW_AVI:
		/*HDMI does not read EDID yet*/
		pr_info("sii8240: %s():%d CEA_NEW_AVI MHL RX Ver.2.x\n",
							__func__, __LINE__);
#ifdef SFEATURE_HDCP_SUPPORT
		sii8240->regs.intr_masks.intr_tpi_mask_value =
		BIT_TPI_INTR_ST0_HDCP_AUTH_STATUS_CHANGE_EVENT |
		BIT_TPI_INTR_ST0_HDCP_VPRIME_VALUE_READY_EVENT |
		BIT_TPI_INTR_ST0_HDCP_SECURITY_CHANGE_EVENT |
		BIT_TPI_INTR_ST0_BKSV_DONE |
		BIT_TPI_INTR_ST0_BKSV_ERR;

		ret = mhl_write_byte_reg(tpi, 0x3C,
			sii8240->regs.intr_masks.intr_tpi_mask_value);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d mhl_write_byte_reg\n",
						__func__, __LINE__);
			goto exit;
		}
#endif
		/*hdmi sink but  avi  => hdmi mode*/
		ret = set_hdmi_mode(sii8240, true);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d set_hdmi_mode\n",
						__func__, __LINE__);
			goto exit;
		}

		if (((sii8240->regs.peer_devcap[MHL_DEVCAP_MHL_VERSION]
						& 0xF0) >= 0x20) &&
		(sii8240->regs.peer_devcap[MHL_DEVCAP_VID_LINK_MODE] &
				 MHL_DEV_VID_LINK_SUPP_PPIXEL) &&
		(sii8240->regs.peer_devcap[MHL_DEVCAP_VID_LINK_MODE] &
				 MHL_DEV_VID_LINK_SUPPYCBCR422)) {
			ret = sii8240_packed_pixel_avi_info_locked(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed!\n",
						__func__, __LINE__);
				goto exit;
			}
		} else {
			ret = sii8240_bypass_avi_info_locked(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed!\n",
						__func__, __LINE__);
				goto exit;
			}
		}

		ret = tmds_control(sii8240, true);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed!\n",
						__func__, __LINE__);
			goto exit;
		}
		break;

	default:
		pr_info("default cmd\n");
		break;
	}
	sii8240->avi_work = false;
	}
	sii8240->avi_cmd = AVI_CMD_NONE;
exit:
	mutex_unlock(&sii8240->lock);
}

static void sii8240_detection_restart(struct work_struct *work)
{
	struct sii8240_data *sii8240 = container_of(work, struct sii8240_data,
						redetect_work);

	mutex_lock(&sii8240->lock);

	pr_info("sii8240: detection restarted\n");
	if (sii8240->pdata->hdmi_mhl_ops) {
		struct msm_hdmi_mhl_ops *hdmi_mhl_ops =	sii8240->pdata->hdmi_mhl_ops;
		hdmi_mhl_ops->set_upstream_hpd(sii8240->pdata->hdmi_pdev, 0);
	}
	if (sii8240->mhl_connected == false) {
		pr_err("[ERROR] sii8240 : already powered off\n");
		goto err_exit;
	}
	sii8240->state = STATE_DISCONNECTED;
	sii8240->rgnd = RGND_UNKNOWN;
	sii8240->ap_hdcp_success = false;
	sii8240->cbus_ready = 0;

	mhl_hpd_control_low(sii8240);

	sii8240->pdata->hw_reset();

	if (sii8240_init_regs(sii8240) < 0) {
		pr_err("[ERROR] sii8240: redetection failed\n");
		goto err_exit;
	}
	if (sii8240_set_mhl_timing(sii8240) < 0) {
		pr_err("[ERROR] sii8240: set mhl timing failed\n");
		goto err_exit;
	}
	if (switch_to_d3(sii8240) < 0) {
		pr_err("[ERROR] sii8240: switch to d3 error\n");
		goto err_exit;
	}
	goto exit;
err_exit:
	if (sii8240->mhl_connected == true) {
		pr_info("still mhl cable connected!!\n");
		queue_work(sii8240->mhl_detection_workqueue,
				&sii8240->redetect_work);
	}
exit:
	mutex_unlock(&sii8240->lock);
}
static void sii8240_power_down_process(struct sii8240_data *sii8240)
{
	pr_info("power_down_process\n");
	sii8240_power_down(sii8240);

	if (sii8240->mhl_connected == true) {
		pr_info("still mhl cable connected!!\n");
		sii8240->pdata->power(1);
		sii8240->cbus_abort = false;
		sii8240->hdmi_sink = false;
		queue_work(sii8240->mhl_detection_workqueue,
				&sii8240->redetect_work);
	}
}

static int sii8240_mhl_onoff(unsigned long event)
{
	int ret;
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);
	int handled = MHL_CON_UNHANDLED;

#ifdef CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK
	pr_info("sii8240_mhl_onoff mmdock debug_log event =  %lu\n", event);
	if (event == 2)	{
		pr_info("sii8240: mmdock connection\n");
		sii8240->pdata->is_multimediadock = true;
		event = 1;
	}
#endif

	if (event == sii8240->muic_state) {
		pr_info("sii8240 : Same muic event, Ignored!\n");
		return MHL_CON_UNHANDLED;
	}

	if (sii8240->pdata->int_gpio_config)
		sii8240->pdata->int_gpio_config(event);

	if (event) {
		pr_info("sii8240:detection started\n");
		wake_lock(&sii8240->mhl_wake_lock);
		sii8240->mhl_connected = true;
		sii8240->muic_state = MHL_ATTACHED;
		sii8240->cbus_ready = 0;
	} else {
		pr_info("sii8240:disconnection\n");
		/* Charging stop when MHL detach */
		if (sii8240->pdata->charger_mhl_cb)
			sii8240->pdata->charger_mhl_cb(false, -1);
		sii8240->mhl_connected = false;
		sii8240->muic_state = MHL_DETACHED;
		wake_unlock(&sii8240->mhl_wake_lock);
		mutex_lock(&sii8240->lock);
#ifdef CONFIG_MUIC_SUPPORT_MULTIMEDIA_DOCK
		sii8240->pdata->is_multimediadock = false;
#endif
		if (sii8240->pdata->hdmi_mhl_ops) {
			struct msm_hdmi_mhl_ops *hdmi_mhl_ops =	sii8240->pdata->hdmi_mhl_ops;
			hdmi_mhl_ops->set_upstream_hpd(sii8240->pdata->hdmi_pdev, 0);
		}

		goto power_down;
	}
	pr_info("lock M--->%d\n", __LINE__);
	mutex_lock(&sii8240->lock);

	/* Reset flags,state of mhl */
	sii8240->state = STATE_DISCONNECTED;
	sii8240->rgnd = RGND_UNKNOWN;
	sii8240->cbus_abort = false;
	sii8240->ap_hdcp_success = false;

	/* Set the board configuration so the  SiI8240 has access to the
	 * external connector
	 */
	if (sii8240->pdata->power) {
		sii8240->pdata->power(1);
		msleep(20);
	}

	if (sii8240->pdata->hw_reset)
		sii8240->pdata->hw_reset();

	ret = sii8240_init_regs(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() - sii8240_init_regs error\n",
								__func__);
		goto unhandled;
	}

	sii8240->hdmi_sink = false;
	ret = sii8240_set_mhl_timing(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: set mhl timing failed\n");
		goto unhandled;
	}

	ret = switch_to_d3(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: switch_to_d3 !\n");
		goto unhandled;
	}

	mutex_unlock(&sii8240->lock);
	pr_info("sii8240: detection_callback return !\n");

	return MHL_CON_HANDLED;

unhandled:
	pr_info("sii8240: Detection failed and additional information about sii8240");
	if (sii8240->state == STATE_DISCONNECTED)
		pr_cont(" (timeout)");
	else if (sii8240->state == STATE_CBUS_UNSTABLE)
		pr_cont(" (cbus unstable)");
	else if (sii8240->state == STATE_NON_MHL_DETECTED)
		pr_cont(" (Non-MHL device(USB?))");
	pr_cont("\n");
power_down:
	mutex_unlock(&sii8240->lock);
	if (sii8240->mhl_event_switch.state == 1) {
		pr_info("sii8240:MHL switch event sent : 0\n");
		switch_set_state(&sii8240->mhl_event_switch, 0);
	}
	sii8240_power_down_process(sii8240);

	return handled;
}

#ifdef CONFIG_EXTCON
static void sii8240_extcon_work(struct work_struct *work)
{
	struct sec_mhl_cable *cable =
		container_of(work, struct sec_mhl_cable, work);

	sii8240_mhl_onoff(cable->cable_state);
}

static int sii8240_extcon_notifier(struct notifier_block *self,
		unsigned long event, void *ptr)
{
	struct sii8240_data *sii8240;
	struct sec_mhl_cable *cable =
		container_of(self, struct sec_mhl_cable, nb);

	if (sii8240_mhldev == NULL) {
		pr_info("%s: sii8240_mhldev is NULL\n", __func__);
		return NOTIFY_DONE;
	}
	sii8240 = dev_get_drvdata(sii8240_mhldev);

	pr_info("%s: '%s' is %s\n", extcon_cable_name[cable->cable_type],
			__func__, event ? "attached" : "detached");

	if (cable->cable_type == EXTCON_MHL) {
		cable->cable_state = event;
		sii8240->pdata->is_smartdock = false;
		schedule_work(&cable->work);
	} else if (cable->cable_type == EXTCON_MHL_VB) {
		/*Here, just notify vbus status to mhl driver.*/
	} else if (cable->cable_type == EXTCON_SMARTDOCK) {
		cable->cable_state = event;
		sii8240->pdata->is_smartdock = true;
		schedule_work(&cable->work);
	}
	return NOTIFY_DONE;
}
#else
static int sii8240_detection_callback(struct notifier_block *this,
					unsigned long event, void *ptr)
{
	return sii8240_mhl_onoff(event);
}
#endif

static int sii8240_discovery_irq_handler(struct sii8240_data *sii8240, u8 intr)
{
	u8 rgnd;
	int ret = 0;
	struct i2c_client *disc = sii8240->pdata->disc_client;

	pr_info("sii8240_discovery_irq_handler : 0x%X\n", intr);
	if (intr & RGND_RDY_INT) {
		ret = mhl_read_byte_reg(disc, DISC_RGND_REG, &rgnd);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: RGND read error: %d\n", ret);
			return ret;
		}
		pr_info("DISC_RGND_REG = 0x%x\n", rgnd);
		switch (rgnd & RGND_INTP_MASK) {
		case RGND_INTP_OPEN:
			pr_info("sii8240: RGND Open\n");
			sii8240->rgnd = RGND_OPEN;
			sii8240->state = STATE_MHL_USB_CONNECTED;
			break;
		case RGND_INTP_1K:
			pr_info("sii8240: RGND 1K\n");
			sii8240->rgnd = RGND_1K;
			sii8240->state = STATE_MHL_CONNECTED;
			break;
		case RGND_INTP_2K:
			pr_info("sii8240: RGND 2K\n");
			sii8240->rgnd = RGND_2K;
			break;
		case RGND_INTP_SHORT:
			pr_info("sii8240: RGND Short\n");
			break;
		};
	} else if (intr & CBUS_UNSTABLE_INT) {
		pr_err("[ERROR] sii8240: CBUS unstable\n");
		sii8240->state = STATE_CBUS_UNSTABLE;
	}
	return ret;
}

static int sii8240_msc_irq_handler(struct sii8240_data *sii8240, u8 intr)
{
	int ret;
	u8 hpd;
	u8 temp;
	u8 cbus_intr[4], cbus_status[4];
	struct i2c_client *cbus = sii8240->pdata->cbus_client;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct cbus_data *data;

	ret = mhl_read_block_reg(cbus, CBUS_MHL_STATUS_OFFSET_0, 4,
				cbus_status);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			 __func__, __LINE__);
		goto err_exit;
	}

	ret = mhl_read_block_reg(cbus, CBUS_MHL_INTR_REG_0, 4, cbus_intr);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			 __func__, __LINE__);
		goto err_exit;
	}

	if (intr & MSC_CMD_DONE) {
		pr_info("sii8240: MSC Command done.ACK Received (intr: 0x%02x)\n", intr);
		complete(&sii8240->cbus_complete);
	}

	if (intr & MSC_HPD_RCVD) {
		pr_info("sii8240: Downstream HPD Changed (intr: 0x%02x)\n", intr);

		ret = mhl_read_byte_reg(cbus, CBUS_CONN_STATUS_REG, &hpd);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() mhl_read_byte_reg : CBUS_CONN_STATUS_REG\n",
				__func__);
			goto err_exit;
		}

		ret = mhl_read_byte_reg(tmds, UPSTRM_HPD_CTRL_REG, &temp);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() mhl_read_byte_reg : CBUS_CONN_STATUS_REG\n",
				__func__);
			goto err_exit;
		}

		if (DOWNSTREAM_HPD_MASK & hpd) {
			pr_info("sii8240: SET_HPD received\n");
			if (temp & HPD_OVERRIDE_EN) {
				/* TODO:If HPD is overriden,//enable HPD_OUT bit
				   in upstream register */
				if (sii8240->hpd_status && platform_hdmi_hpd_status()) {
					pr_err("[ERROR] sii8240: hpd_status already true\n");
					goto err_exit;
				}
				sii8240->hpd_status = true;
				sii8240->avi_cmd = HPD_HIGH_EVENT;
				sii8240->avi_work = true;
				queue_work(sii8240->avi_cmd_wqs,
					&sii8240->avi_control_work);
				/*Initiate sii8240's information variables*/
				memset(&sii8240->current_aviInfoFrame,
					0x00, INFO_BUFFER);
				memset(&sii8240->output_avi_data,
					0x00, SIZE_AVI_INFOFRAME);
				memset(&sii8240->vendorSpecificInfoFrame,
					0x00, INFO_BUFFER);
			}
		} else {
			pr_info("sii8240: CLR_HPD received\n");
			if (temp & HPD_OVERRIDE_EN) {
				/* TODO:If HPD is overriden,clear HPD_OUT bit
				   upstream register */
				if (sii8240->pdata->hdmi_mhl_ops) {
					struct msm_hdmi_mhl_ops *hdmi_mhl_ops =	sii8240->pdata->hdmi_mhl_ops;
					hdmi_mhl_ops->set_upstream_hpd(sii8240->pdata->hdmi_pdev, 0);
				}

				sii8240->hpd_status = false;
				sii8240->tmds_enable = false;
				sii8240->ap_hdcp_success = false;
				sii8240->cbus_ready = 0;
				ret = mhl_modify_reg(tmds, UPSTRM_HPD_CTRL_REG,
					BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK|
					BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK,
					BIT_HPD_CTRL_HPD_OUT_OVR_VAL_OFF|
					BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: %s():%d failed !\n",
							__func__, __LINE__);
					goto err_exit;
				}
				/*Disable interrupt*/
				sii8240->regs.intr_masks.intr7_mask_value = 0;
				sii8240->regs.intr_masks.intr8_mask_value = 0;

				ret = mhl_write_byte_reg(tmds, 0x7D,
				sii8240->regs.intr_masks.intr7_mask_value);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: %s():%d failed !\n",
							__func__, __LINE__);
					goto err_exit;
				}

				ret = mhl_write_byte_reg(tmds, 0x7E,
				sii8240->regs.intr_masks.intr8_mask_value);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] sii8240: %s():%d failed !\n",
							__func__, __LINE__);
					goto err_exit;
				}

				ret = set_mute_mode(sii8240, true);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] %s() set_mute_mode fail %d\n",
						__func__, __LINE__);
					goto err_exit;
				}
				ret = tmds_control(sii8240, false);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] %s() tmds_control\n",
							__func__);
					goto err_exit;
				}
			}
			if (sii8240->mhl_event_switch.state == 1) {
				pr_info("sii8240: MHL switch event sent :0\n");
				switch_set_state(&sii8240->mhl_event_switch, 0);
			}
		}
	}

	if (intr & MSC_WRITE_STAT_RCVD) {
		bool path_en_changed = false;
		pr_info("sii8240: WRITE_STAT received\n");

		sii8240->cbus_ready = cbus_status[0] & MHL_STATUS_DCAP_READY;
		if (sii8240->cbus_ready)
			pr_info("sii8240: DCAP_READY intr\n");

		if (!(sii8240->regs.link_mode & MHL_STATUS_PATH_ENABLED) &&
			(MHL_STATUS_PATH_ENABLED & cbus_status[1])) {

			/* PATH_EN{SOURCE} = 0 and PATH_EN{SINK}= 1 */
			sii8240->regs.link_mode |= MHL_STATUS_PATH_ENABLED;
			path_en_changed = true;
		} else if ((sii8240->regs.link_mode & MHL_STATUS_PATH_ENABLED)
			&& !(MHL_STATUS_PATH_ENABLED & cbus_status[1])) {

			/* PATH_EN{SOURCE} = 1 and PATH_EN{SINK}= 0 */
			sii8240->regs.link_mode &= ~MHL_STATUS_PATH_ENABLED;
			path_en_changed = true;
			ret = tmds_control(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() tmds_control\n",
					__func__);
				goto err_exit;
			}
		}
		if (path_en_changed)
			sii8240_queue_cbus_cmd_locked(sii8240, WRITE_STAT,
					CBUS_MHL_STATUS_OFFSET_1,
					sii8240->regs.link_mode);
	}

	if (intr & MSC_MSG_RCVD) {
		pr_info("sii8240: MSC_MSG received\n");
		data = kzalloc(sizeof(struct cbus_data), GFP_KERNEL);
		if (!data) {
			pr_err("[ERROR] sii8240: failed to allocate cbus data\n");
			ret = -ENOMEM;
			goto err_exit;
		}
		data->cmd = MSC_MSG;
		ret = mhl_read_byte_reg(cbus, MSC_MSG_RCVD_DATA1_REG, &data->offset);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() mhl_read_byte_reg : MSC_MSG_RCVD_DATA1_REG\n",
				__func__);
			goto err_exit;
		}

		mhl_read_byte_reg(cbus, MSC_MSG_RCVD_DATA2_REG, &data->data);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() mhl_read_byte_reg : MSC_MSG_RCVD_DATA2_REG\n",
				__func__);
			goto err_exit;
		}
		list_add_tail(&data->list, &sii8240->cbus_data_list);
		queue_work(sii8240->cbus_cmd_wqs, &sii8240->cbus_work);
	}

	if (intr & MSC_WRITE_BURST_RCVD)
		pr_info("sii8240: WRITE_BURST received\n");

	if (intr & MSC_SET_INT_RCVD) {
		pr_info("sii8240: SET_INT received\n");
		if (cbus_intr[0] & MHL_INT_DCAP_CHG) {
			pr_info("sii8240: device capability changed\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_MHL_VERSION, 0) < 0)
				pr_info("sii8240: MHL_VERSION read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_ADOPTER_ID_H, 0) < 0)
				pr_info("sii8240: MHL_ADOPTER_ID_H read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_ADOPTER_ID_L, 0) < 0)
				pr_info("sii8240: MHL_ADOPTER_ID_L read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_RESERVED, 0) < 0)
				pr_info("sii8240: MHL_RESERVED read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_DEV_CAT, 0) < 0)
				pr_info("sii8240: DEVCAP_DEV_CAT read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_FEATURE_FLAG, 0) < 0)
				pr_info("sii8240: FEATURE_FLAG read fail\n");
			if (sii8240_queue_cbus_cmd_locked(sii8240, READ_DEVCAP,
					MHL_DEVCAP_VID_LINK_MODE, 0) < 0)
				pr_info("sii8240: VID_LINK_MODE read fail\n");
		}

		if (cbus_intr[0] & MHL_INT_DSCR_CHG) {
			pr_info("sii8240: scratchpad register change done\n");
			ret = sii8240_get_write_burst(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() sii8240_get_write_burst\n", __func__);
				goto err_exit;
			}
		}

		if (cbus_intr[0] & MHL_INT_REQ_WRT) {
			pr_info("sii8240: request-to-write received\n");

			data = kzalloc(sizeof(struct cbus_data), GFP_KERNEL);
			if (!data) {
				pr_err("[ERROR] sii8240: failed to allocate cbus data\n");
				ret = -ENOMEM;
				goto err_exit;
			}
			data->cmd = SET_INT;
			data->offset = CBUS_MHL_INTR_REG_0;
			data->data = MHL_INT_GRT_WRT;
			list_add_tail(&data->list, &sii8240->cbus_data_list);

			queue_work(sii8240->cbus_cmd_wqs, &sii8240->cbus_work);
		}

		if (cbus_intr[0] & MHL_INT_GRT_WRT)
			pr_info("sii8240: grant-to-write received\n");

		if (cbus_intr[1] & MHL_INT_EDID_CHG) {
			sii8240_toggle_hpd(cbus);
			/*Initiate sii8240's information variables*/
			sii8240->hpd_status = false;
			memset(&sii8240->current_aviInfoFrame,
				0x00, INFO_BUFFER);
			memset(&sii8240->output_avi_data,
				0x00, SIZE_AVI_INFOFRAME);
			memset(&sii8240->vendorSpecificInfoFrame,
				0x00, INFO_BUFFER);

			/*Disable interrupt*/
			sii8240->regs.intr_masks.intr7_mask_value = 0;
			sii8240->regs.intr_masks.intr8_mask_value = 0;
			ret = mhl_write_byte_reg(tmds, 0x7D,
				sii8240->regs.intr_masks.intr7_mask_value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
				goto err_exit;
			}
			ret = mhl_write_byte_reg(tmds, 0x7E,
				sii8240->regs.intr_masks.intr8_mask_value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
				goto err_exit;
			}
			ret = set_mute_mode(sii8240, true);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() set_mute_mode fail %d\n",
						__func__, __LINE__);
				goto err_exit;
			}
			ret = tmds_control(sii8240, false);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() tmds_control\n",
					__func__);
				goto err_exit;
			}

			/*After HPD toggle, Need to setup again*/
			sii8240->hpd_status = true;
			sii8240->avi_cmd = HPD_HIGH_EVENT;
			sii8240->avi_work = true;
			queue_work(sii8240->avi_cmd_wqs,
					&sii8240->avi_control_work);
			/*Wake up avi control thread*/
		}
	}

	if (intr & MSC_MSG_NACK_RCVD)
		pr_info(KERN_ERR "NACK received\n");
err_exit:
	return ret;
}

/*Need to check*/
static int sii8240_msc_error_irq_handler(struct sii8240_data *sii8240, u8 intr)
{
	int ret = 0;
	u8 error;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;

	if (intr == 0)
		return ret;

	if (intr & SEND_ERROR_INT) {
		ret = mhl_read_byte_reg(cbus, MSC_SEND_ERROR_REG, &error);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			goto err_exit;
		}
		if (error & CMD_ABORT) {
			pr_warn("[WARN] sii8240: [sender] Command Aborted\n");
			sii8240->cbus_abort = true;
		}
		if (error & CMD_UNDEF_OPCODE)
			pr_warn("[WARN] sii8240: [sender] Undefined Opcode\n");
		if (error & CMD_TIMEOUT)
			pr_warn("[WARN] sii8240: [sender] Timeout\n");
		if (error & CMD_RCVD_PROT_ERR)
			pr_warn("[WARN] sii8240: [sender] Protocol Error\n");
		if (error & CMD_MAX_FAIL)
			pr_warn("[WARN] sii8240:[sender]Failed MAX retries\n");
	}
	if (intr & RECD_ERROR_INT) {
		ret = mhl_read_byte_reg(cbus, MSC_RECVD_ERROR_REG, &error);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
			goto err_exit;
		}
		if (error & CMD_ABORT) {
			pr_warn("[WARN] sii8240: [receiver] Command Aborted\n");
			sii8240->cbus_abort = true;
		}
		if (error & CMD_UNDEF_OPCODE)
			pr_warn("[WARN] sii8240: [receiver] Undefined Opcode\n");
		if (error & CMD_TIMEOUT)
			pr_warn("[WARN] sii8240: [reciever] Timeout\n");
		if (error & CMD_RCVD_PROT_ERR)
			pr_warn("[WARN]sii8240: [reciever] Protocol Error\n");
		if (error & CMD_MAX_FAIL)
			pr_warn("[WARN] sii8240:[reciever]Command Failed After MAX\n");

	}
	if (intr & RECD_DDC_ABORT_INT) {
		/* TODO: We used to terminate connection after
		 * DDC_ABORT had been received.Need more details on how to
		 * handle this interrupt with MHL 1.2 specs. */
		pr_warn("[WARN] sii8240: DDC_ABORT received\n");
	}

err_exit:
	return ret;
}
static int sii8240_rx_connected(struct sii8240_data *sii8240,
		u8 int1, u8 cbus_con_st)
{
	int ret = 0;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	struct i2c_client *disc = sii8240->pdata->disc_client;
	struct cbus_data *cbus_cmd;

	sii8240->cbus_connected = (cbus_con_st & 0x01);

	if ((int1 & BIT_INTR4_MHL_EST) && (sii8240->cbus_connected)) {

		sii8240->state = STATE_MHL_DISCOVERY_SUCCESS;

		ret = mhl_hpd_control_low(sii8240);
		if (unlikely(ret < 0))
			pr_warn("[WARN]sii8240: %s():%d failed !\n",
					__func__, __LINE__);

		/*Must be set to 1.*/
		ret = mhl_write_byte_reg(hdmi, MHLTX_TERM_CTRL_REG,
				BIT_MHLTX_CTL1_DISC_OVRIDE_ON);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
			return ret;
		}

		/*Enable discovery*/
		pr_info("sii8240:Enable discovery\n");
		ret = mhl_modify_reg(disc, 0x10, 0x01, 0x01);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
			return ret;
		}

		memset(&sii8240->regs.intr_masks, 0,
			sizeof(sii8240->regs.intr_masks));
		sii8240->regs.intr_masks.intr1_mask_value =
			BIT_INTR1_HPD_CHG;
		sii8240->regs.intr_masks.intr4_mask_value =
			(BIT_INTR4_CBUS_LKOUT | BIT_INTR4_CBUS_DISCONNECT);
		sii8240->regs.intr_masks.intr_cbus0_mask_value =
			(BIT_CBUS_MSC_MT_DONE |
			 BIT_CBUS_HPD_RCVD |
			 BIT_CBUS_MSC_MR_WRITE_STAT |
			 BIT_CBUS_MSC_MR_MSC_MSG |
			 BIT_CBUS_MSC_MR_WRITE_BURST |
			 BIT_CBUS_MSC_MR_SET_INT |
			 BIT_CBUS_MSC_MT_DONE_NACK);

		sii8240->regs.intr_masks.intr_cbus1_mask_value =
		(BIT_CBUS_DDC_ABRT | BIT_CBUS_CEC_ABRT | BIT_CBUS_CMD_ABORT);

		ret = sii8240_set_interrupt(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() sii8240_set_interrupt\n", __func__);
			return ret;
		}

		/*CBUS command*/
		cbus_cmd = kzalloc(sizeof(struct cbus_data), GFP_KERNEL);
		if (!cbus_cmd) {
			pr_err("[ERROR] sii8240: failed to allocate cbus data\n");
			return -ENOMEM;
		}

		cbus_cmd->cmd = WRITE_STAT;
		cbus_cmd->offset = CBUS_MHL_STATUS_OFFSET_0;
		cbus_cmd->data = MHL_STATUS_DCAP_READY;
		list_add_tail(&cbus_cmd->list, &sii8240->cbus_data_list);

		pr_debug("schedule_work: cbus_work\n");
		queue_work(sii8240->cbus_cmd_wqs, &sii8240->cbus_work);

	} else if (int1 & BIT_INTR4_NON_MHL_EST) {
		pr_info("sii8240: discovery failed\n");
		sii8240->state = STATE_MHL_DISCOVERY_FAIL;

	} else if (sii8240->cbus_connected) {
		sii8240->state = STATE_MHL_DISCOVERY_ON;
		memset(&sii8240->regs.intr_masks, 0,
			sizeof(sii8240->regs.intr_masks));
		sii8240->regs.intr_masks.intr4_mask_value =
			(BIT_INTR4_MHL_EST|BIT_INTR4_NON_MHL_EST);
		sii8240->regs.intr_masks.intr_cbus1_mask_value =
			(BIT_CBUS_DDC_ABRT | BIT_CBUS_CEC_ABRT |
						BIT_CBUS_CMD_ABORT);

		ret = sii8240_set_interrupt(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() sii8240_set_interrupt\n", __func__);
			return ret;
		}
	}

	return ret;
}

static int sii8240_cbus_irq(struct sii8240_data *sii8240)
{
	struct i2c_client *cbus = sii8240->pdata->cbus_client;
	int ret;
	u8 cbus_con_st = 0, msc_intr = 0,
		msc_intr_en = 0, msc_err_intr = 0, msc_err_en = 0;

	ret = mhl_read_byte_reg(cbus, CBUS_CONN_STATUS_REG, &cbus_con_st);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	ret = mhl_read_byte_reg(cbus, CBUS_MSC_INTR_REG, &msc_intr);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	ret = mhl_read_byte_reg(cbus, CBUS_MSC_INTR_ENABLE_REG, &msc_intr_en);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	ret = mhl_read_byte_reg(cbus, CBUS_MSC_ERROR_INTR_REG, &msc_err_intr);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
		return ret;
	}
	ret = mhl_read_byte_reg(cbus, CBUS_MSC_ERROR_INTR_ENABLE_REG,
								&msc_err_en);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
		return ret;
	}

	if (msc_err_intr) {
		ret = mhl_write_byte_reg(cbus, CBUS_MSC_ERROR_INTR_REG, msc_err_intr);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
			return ret;
		}
	}

	if (msc_intr) {
		ret = mhl_write_byte_reg(cbus, CBUS_MSC_INTR_REG, msc_intr);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s, %d\n", __func__, __LINE__);
			return ret;
		}
	}

	/*Handling of Cbus error*/
	ret = sii8240_msc_error_irq_handler(sii8240, msc_err_intr);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
			__func__, __LINE__);
		return ret;
	}
	ret = sii8240_msc_irq_handler(sii8240, msc_intr);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR]sii8240: %s():%d failed !\n",
			__func__, __LINE__);
		return ret;
	}

	return ret;
}
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
static void sii8240_avi_check_work(struct work_struct *work)
{
	struct sii8240_data *sii8240 = container_of(work, struct sii8240_data,
								avi_check_work);
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
	int ret;
	u8 checksum = 0;

	pr_info("Sii8240: AVIF getting problem??\n");
	ret = sii8240_get_avi_info(sii8240);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
					__func__, __LINE__);
		return;
	}

	if ((sii8240->aviInfoFrame[0] != 0x82) ||
		(sii8240->aviInfoFrame[1] != 0x02) ||
		(sii8240->aviInfoFrame[2] != 0x0D)) {
		pr_info("Sii8240: NO AVIF.:%d\n", __LINE__);

		ret = mhl_modify_reg(tpi, 0x1A,
			TMDS_OUTPUT_CONTROL_MASK,
			TMDS_OUTPUT_CONTROL_ACTIVE);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
			return;
		}

		ret = mhl_modify_reg(tpi, 0x1A,
			TMDS_OUTPUT_CONTROL_MASK,
			TMDS_OUTPUT_CONTROL_POWER_DOWN);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
			return;
		}
		return;
	}

	if (sii8240_check_avi_info(sii8240)) {
		pr_info("sii8240: AVI change! %d\n", __LINE__);
		checksum = sii8240_check_avi_info_checksum
				(sii8240, sii8240->aviInfoFrame);

		if (checksum) {
			pr_err("[ERROR] sii8240: %s():%d checksum error!\n",
						__func__, __LINE__);
			return;
		}

		memcpy(&sii8240->current_aviInfoFrame,
				&sii8240->aviInfoFrame, INFO_BUFFER);
		pr_info("Sii8240: avi change and tmds off\n");
		ret = tmds_control(sii8240, false);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed !\n",
						__func__, __LINE__);
			return;
		}

		sii8240->avi_cmd = CEA_NEW_AVI;
		sii8240->avi_work = true;
		queue_work(sii8240->avi_cmd_wqs,
				&sii8240->avi_control_work);
	} else
		pr_info("sii8240: %s():%d NO AVI change!!\n",
						__func__, __LINE__);
}

static void sii8240_avif_check_callback(unsigned long data)
{
	struct sii8240_data *sii8240 = (struct sii8240_data *) data;

	if (((sii8240->aviInfoFrame[0] == 0x82) &&
		(sii8240->aviInfoFrame[1] == 0x02) &&
		(sii8240->aviInfoFrame[2] == 0x0D)) ||
			(sii8240->hdmi_sink == false)) {
		pr_info("Sii8240: %s() return AVIF callback\n", __func__);
	} else
		queue_work(sii8240->avi_cmd_wqs, &sii8240->avi_check_work);
}

static int sii8240_check_ckdt_change(struct sii8240_data *sii8240)
{
	int ret = 0;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	if (sii8240->r281 == 0) {
		pr_info("Sii8240: %s():%d  (r281 == 0)\n", __func__, __LINE__);
		ret = mhl_read_byte_reg(hdmi, 0x81, &sii8240->r281);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
						__func__, __LINE__);
			return ret;
		}
	}

	pr_info("Sii8240: %s():%d r281 data: %02x\n",
				__func__, __LINE__, sii8240->r281);

	ret = mhl_write_byte_reg(hdmi, 0x81, (sii8240->r281 & (~0x3F)));
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed\n",
			__func__, __LINE__);
		return ret;
	}

	if (sii8240->r281 & 0x3F) {
		/* there is default value...*/
		pr_info("Sii8240: there is default value.\n");
	} else {
		sii8240->r281 = 0xFF;
		pr_info("Sii8240: there is non default value.\n");
	}

	ret = mhl_modify_reg(tmds, 0x80, BIT_TMDS_CCTRL_TMDS_OE,
						BIT_TMDS_CCTRL_TMDS_OE);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed !\n",
			__func__, __LINE__);
		return ret;
	}
	return ret;
}

static int sii8240_check_scdt_reg(struct sii8240_data *sii8240)
{
	int ret = 0;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;

	if (sii8240->r281 != 0) {
		pr_info("Sii8240: r281 == 0x%02x\n", sii8240->r281);
		ret = mhl_modify_reg(tmds, 0x80, BIT_TMDS_CCTRL_TMDS_OE, 0x0);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
							__func__, __LINE__);
			return ret;
		}
		if (sii8240->r281 == 0xFF)
			sii8240->r281 = 0;

		ret = mhl_write_byte_reg(hdmi, 0x81, sii8240->r281);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
							__func__, __LINE__);
			return ret;
		}
		sii8240->r281 = 0;
	}
	mod_timer(&sii8240->avi_check_timer, jiffies + msecs_to_jiffies(200));

	return ret;
}

#endif
static int sii8240_audio_video_intr_control(struct sii8240_data *sii8240)
{
	int ret;
	u8 upstatus, ceainfo;
	u8 checksum = 0;
#ifdef SFEATURE_HDCP_SUPPORT
	u8 value;
	struct i2c_client *tpi = sii8240->pdata->tpi_client;
#endif
	struct i2c_client *tmds = sii8240->pdata->tmds_client;

	pr_info("sii8240_audio_video_intr_control\n");
#ifdef SFEATURE_HDCP_SUPPORT
/*TO do*/
	ret = mhl_read_byte_reg(tpi, HDCP_INTR , &value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed\n",
				__func__, __LINE__);
		return ret;
	}

	ret = mhl_write_byte_reg(tpi, HDCP_INTR , value);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed\n",
				__func__, __LINE__);
		return ret;
	}

	if (value && sii8240->hpd_status && sii8240->tmds_enable) {
		ret = sii8240_hdcp_control(sii8240, value);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
			return ret;
		}
	}
#endif
	ret = mhl_read_byte_reg(tmds, US_INTR, &upstatus);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed\n",
				__func__, __LINE__);
		return ret;
	}

	ret = mhl_read_byte_reg(tmds, 0x7C, &ceainfo);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: %s():%d failed\n",
				__func__, __LINE__);
		return ret;
	}

	if (upstatus) {
		ret = mhl_write_byte_reg(tmds, US_INTR, upstatus);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
			return ret;
		}
	}
	if (ceainfo) {
		ret = mhl_write_byte_reg(tmds, 0x7C, ceainfo);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
			return ret;
		}
	}

	if ((sii8240->regs.intr_masks.intr7_mask_value & BIT_INTR7_CEA_NO_AVI)
					&& (upstatus & BIT_INTR7_CEA_NO_AVI))
		pr_info("sii8240: NO_AVI_INFORMATION\n");
	if (upstatus & BIT_INTR7_CEA_NO_VSI) {
		pr_info("sii8240: BIT_INTR7_CEA_NO_VSI\n");
#ifdef MHL_2X_3D
		sii8240->input_3d_format = NON_3D_VIDEO;
		ret = mhl_modify_reg(tmds, 0x51, BIT_VID_OVRRD_3DCONV_EN_MASK,
						BIT_VID_OVRRD_3DCONV_EN_NORMAL);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
			return ret;
		}
#endif
	}
	if (upstatus & BIT_INTR7_CP_NEW_CP)
		pr_info("sii8240: BIT_INTR7_CP_NEW_CP\n");
	if (upstatus & BIT_INTR7_CP_SET_MUTE) {
		pr_info("sii8240: BIT_INTR7_CP_SET_MUTE\n");
		ret = set_mute_mode(sii8240, true);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: get avi info error\n");
			return ret;
		}
	}
	if (upstatus & BIT_INTR7_CP_CLR_MUTE) {
		pr_info("sii8240: BIT_INTR7_CP_CLR_MUTE\n");
		ret = set_mute_mode(sii8240, false);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: get avi info error\n");
			return ret;
		}
	}

#ifdef MHL_2X_3D
	/*VSI 3D*/
	if ((ceainfo & BIT_INTR8_CEA_NEW_VSI) &&
		(sii8240->hpd_status == true)) {
		ret = sii8240_get_vsi_info(sii8240);
		if (ret) {
			pr_err("[ERROR] sii8240: %s():%d sii8240_get_vsi_info\n",
					__func__, __LINE__);
			return ret;
		}
		ret = sii8240_check_avi_info_checksum(sii8240,
			sii8240->vendorSpecificInfoFrame);
		if (ret) {
			pr_err("[ERROR] vendor specific checksum failing\n");
			return ret;
		}
		print_hex_dump(KERN_ERR, "vsi = ",
			DUMP_PREFIX_NONE, 16, 1,
			sii8240->vendorSpecificInfoFrame, 16, false);
		if ((sii8240->vendorSpecificInfoFrame[7] & 0xE0) == 0x40) {
			if ((sii8240->vendorSpecificInfoFrame[8]
				& 0xF0) == 0x00) {

				/*frame packing 3D*/
			pr_info("sii8240: frame packing 3D\n");
			sii8240->input_3d_format = FRAME_PACKING_3D;
			ret = mhl_modify_reg(tmds, 0x51,
				BIT_VID_OVRRD_3DCONV_EN_MASK,
				BIT_VID_OVRRD_3DCONV_EN_FRAME_PACK);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
						__func__, __LINE__);
				return ret;
			}
			} else {
			/*non frame packing 3D*/
			pr_info("sii8240:non frame packing 3D\n");
			sii8240->input_3d_format = NON_FRAME_PACKING_3D;
			ret = mhl_modify_reg(tmds, 0x51,
				BIT_VID_OVRRD_3DCONV_EN_MASK,
				BIT_VID_OVRRD_3DCONV_EN_NORMAL);
			}
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
						__func__, __LINE__);
				return ret;
			}
		}
	}
#endif
	/*AVI InfoFrame*/
	if ((ceainfo & BIT_INTR8_CEA_NEW_AVI) &&
			(sii8240->hpd_status == true)) {
		pr_info("sii8240 : BIT_INTR8_CEA_NEW_AVI\n");
		ret = sii8240_get_avi_info(sii8240);
		if (unlikely(ret < 0)) {
			pr_info("sii8240 : get avi info error\n");
			return ret;
		}
		if (sii8240_check_avi_info(sii8240)) {
			pr_info("sii8240: %s():%d AVI change!\n",
						__func__, __LINE__);
		checksum = sii8240_check_avi_info_checksum
				(sii8240, sii8240->aviInfoFrame);
		if (checksum) {
			pr_info("sii8240: %s():%d checksum error\n",
						__func__, __LINE__);
			return -EINVAL;
		}
		memcpy(&sii8240->current_aviInfoFrame,
				&sii8240->aviInfoFrame, INFO_BUFFER);

		tmds_control(sii8240, false);
		sii8240->avi_cmd = CEA_NEW_AVI;
		sii8240->avi_work = true;
		queue_work(sii8240->avi_cmd_wqs, &sii8240->avi_control_work);
		} else
			pr_info("sii8240: no AVIF\n");
	}

	return 0;
}

static irqreturn_t sii8240_irq_thread(int irq, void *data)
{
	struct sii8240_data *sii8240 = data;
	struct i2c_client *disc = sii8240->pdata->disc_client;
	struct i2c_client *cbus = sii8240->pdata->cbus_client;
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	struct i2c_client *hdmi = sii8240->pdata->hdmi_client;
	u8 intr1 = 0, intr1_en, int1_status, int2_status, cbus_con_st;
	u8 value;
	u8 cbus1, cbus2, cbus3, cbus4;
	int ret;
	bool clock_stable = false;

	mutex_lock(&sii8240->lock);
	if (sii8240->state == STATE_DISCONNECTED) {
		pr_info("sii8240: mhl is disconnected, so return irq\n");
		mutex_unlock(&sii8240->lock);
		return IRQ_HANDLED;
	}
	ret = mhl_read_byte_reg(disc, DISC_INTR_REG, &intr1);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : DISC_INTR_REG\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(disc, DISC_INTR_ENABLE_REG, &intr1_en);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : DISC_INTR_ENABLE_REG\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(cbus, CBUS_CONN_STATUS_REG, &cbus_con_st);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : CBUS_CONN_STATUS_REG\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(tmds, 0x71, &int1_status);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : 0x71\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(tmds, 0x74, &int2_status);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : 0x74\n", __func__);
		goto err_exit2;
	}

	/*Cbus link interrupt*/
	ret = mhl_read_byte_reg(cbus, 0x20, &cbus1);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : 0x20\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(cbus, 0x21, &cbus2);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : 0x21\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(cbus, 0x22, &cbus3);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : 0x22\n", __func__);
		goto err_exit2;
	}
	ret = mhl_read_byte_reg(cbus, 0x23, &cbus4);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_read_byte_reg : 0x23\n", __func__);
		goto err_exit2;
	}

	switch (sii8240->state) {
	case STATE_MHL_READY_RGND_DETECT:
		pr_info(KERN_INFO "sii8240:MHL_READY_RGND_DETECT\n");
		/* interrupts related to discovery & connection */
		if (intr1) {
			pr_info("sii8240: discovery irq %02x/%02x\n",
					intr1, intr1_en);
			ret = sii8240_discovery_irq_handler(sii8240, intr1);
				if (unlikely(ret < 0)) {
					pr_err("[ERROR] %s() sii8240_discovery_irq_handler\n", __func__);
					goto err_exit;
				}
			}
		if (sii8240->state == STATE_MHL_CONNECTED) {
			if (sii8240->pdata->charger_mhl_cb)
				sii8240->pdata->charger_mhl_cb(true, 0x03);
			ret = sii8240_init_regs(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() sii8240_init_regs\n", __func__);
				goto err_exit;
			}
			memset(&sii8240->regs.intr_masks,
					0x0, sizeof(sii8240->regs.intr_masks));
			sii8240->regs.intr_masks.intr4_mask_value =
				(BIT_INTR4_MHL_EST|BIT_INTR4_NON_MHL_EST);
			ret = sii8240_set_interrupt(sii8240);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() sii8240_set_interrupt\n", __func__);
				goto err_exit;
			}
		} else if (intr1 & (RGND_RDY_INT | CBUS_UNSTABLE_INT)) {
			pr_info("sii8240: rgnd 1k not detected\n");
			if (sii8240->irq_enabled) {
				disable_irq_nosync(sii8240->irq);
				sii8240->irq_enabled = false;
				pr_info("sii8240: interrupt disabled\n");
			}
			/* If there is VBUS, charging start */
			if (sii8240->pdata->vbus_present)
				if (sii8240->pdata->vbus_present()) {
					if (sii8240->pdata->charger_mhl_cb)
						sii8240->pdata->charger_mhl_cb(false, 0x03);
				}
			queue_work(sii8240->mhl_detection_workqueue,
						 &sii8240->redetect_work);
		}
		break;
	case STATE_MHL_CONNECTED:
		pr_info("sii8240:MHL_RX_CONNECTED\n");
		ret = sii8240_rx_connected(sii8240, intr1, cbus_con_st);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] %s() sii8240_rx_connected\n", __func__);
			goto err_exit;
		}

		if (sii8240->state == STATE_MHL_DISCOVERY_FAIL) {
				/*changing of status information*/
			if (sii8240->irq_enabled) {
				disable_irq_nosync(sii8240->irq);
				sii8240->irq_enabled = false;
				pr_info("sii8240: interrupt disabled\n");
			}
			/* CTS 3.3.5.2 */
			/* OTG should turn off when discovery fail */
			if(sii8240->pdata->charging_type== POWER_SUPPLY_TYPE_OTG)
				if (sii8240->pdata->charger_mhl_cb)
					sii8240->pdata->charger_mhl_cb(false, -1);
			queue_work(sii8240->mhl_detection_workqueue,
						 &sii8240->redetect_work);
		}
		break;
	case STATE_MHL_DISCOVERY_SUCCESS:
		pr_debug(KERN_INFO "sii8240:MHL_DISCOVERY_SUCCESS\n");

		/*checking of cbus locking out*/
		if (intr1 & BIT_INTR4_CBUS_LKOUT) {
			force_usb_id_switch_open(sii8240);
			release_usb_id_switch_open(sii8240);
			break;
		}
		/*checking of cbus disconnection*/
		if (intr1 & BIT_INTR4_CBUS_DISCONNECT) {
			pr_info("sii8240 : cbus_disconnected\n");
			/* CTS 3.3.22.3 */
			/* If cbus disconnected, OTG should also be stoped */
			if(sii8240->pdata->charging_type== POWER_SUPPLY_TYPE_OTG)
				if (sii8240->pdata->charger_mhl_cb)
					sii8240->pdata->charger_mhl_cb(false, -1);

			ret = mhl_write_byte_reg(hdmi, 0x80, 0xD0);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
						__func__, __LINE__);
				goto err_exit;
			}
			pr_info("sii8240: CBUS DISCONNECTED !!\n");
			if (sii8240->irq_enabled) {
				disable_irq_nosync(sii8240->irq);
				sii8240->irq_enabled = false;
				pr_info("sii8240: interrupt disabled\n");
			}
			queue_work(sii8240->mhl_detection_workqueue,
						&sii8240->redetect_work);
			break;
		}
		if (int2_status == 0x00) {
			sii8240->regs.intr_masks.intr1_mask_value =
				(BIT_INTR1_HPD_CHG);
			ret = mhl_write_byte_reg(tmds, 0x75,
				sii8240->regs.intr_masks.intr1_mask_value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
						__func__, __LINE__);
				goto err_exit;
			}
		}

		/*checking HPD event high/low*/
		if (int1_status & BIT_INTR1_HPD_CHG) {
			ret = mhl_read_byte_reg
				(tmds, MHL_TX_SYSSTAT_REG, &value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] sii8240: %s():%d failed\n",
						__func__, __LINE__);
				goto err_exit;
			}
			memset(&sii8240->current_aviInfoFrame,
					0x00, INFO_BUFFER);
			memset(&sii8240->output_avi_data,
					0x00, SIZE_AVI_INFOFRAME);
#ifdef MHL_2X_3D
		memset(&sii8240->vendorSpecificInfoFrame,
					0x00, INFO_BUFFER);
		sii8240->input_3d_format = NON_3D_VIDEO;
		ret = mhl_modify_reg(tmds, 0x51,
				BIT_VID_OVRRD_3DCONV_EN_MASK,
				BIT_VID_OVRRD_3DCONV_EN_NORMAL);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240: %s():%d failed\n",
					__func__, __LINE__);
			goto err_exit;
		}
#endif

			if (value & 0x02) {
				pr_info("HPD event high\n");
				if (sii8240->hpd_status == false) {
					sii8240->hpd_status = true;
					sii8240->avi_cmd = HPD_HIGH_EVENT;
					sii8240->avi_work = true;
					queue_work(sii8240->avi_cmd_wqs,
						&sii8240->avi_control_work);
				}
			} else {
				pr_info("Sii8240:HPD event low\n");
				if (sii8240->hpd_status) {
					if (sii8240->pdata->hdmi_mhl_ops) {
						struct msm_hdmi_mhl_ops *hdmi_mhl_ops =	sii8240->pdata->hdmi_mhl_ops;
						hdmi_mhl_ops->set_upstream_hpd(sii8240->pdata->hdmi_pdev, 0);
					}

					sii8240->hpd_status = false;
					tmds_control(sii8240, false);
					ret = mhl_modify_reg(tmds,
						UPSTRM_HPD_CTRL_REG,
					BIT_HPD_CTRL_HPD_OUT_OVR_VAL_MASK |
					BIT_HPD_CTRL_HPD_OUT_OVR_EN_MASK,
					BIT_HPD_CTRL_HPD_OUT_OVR_VAL_OFF |
					BIT_HPD_CTRL_HPD_OUT_OVR_EN_ON);
					if (unlikely(ret < 0)) {
						pr_err("[ERROR] sii8240: %s():%d failed\n",
								__func__, __LINE__);
						goto err_exit;
					}
					sii8240->regs.intr_masks.intr5_mask_value = 0x00;
					ret = mhl_write_byte_reg(tmds, 0x78, 0x00);
					if (unlikely(ret < 0)) {
						pr_err("[ERROR] sii8240: %s():%d failed\n",
								__func__, __LINE__);
						goto err_exit;
					}
					ret = mhl_modify_reg(hdmi, 0xA1,
						BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_MASK,
						BIT_REG_RX_HDMI_CTRL0_hdmi_mode_overwrite_HW_CTRL);
					if (unlikely(ret < 0)) {
						pr_err("[ERROR] sii8240: %s():%d failed\n",
								__func__, __LINE__);
						goto err_exit;
					}
				}
			}
		}
		/*Upstream TMDS interrupt register*/
		if (int2_status & BIT_INTR5_CKDT_CHANGE) {
			pr_info("sii8240: CKDT:"\
					"Upstream TMDS interrupt register\n");
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
			if (sii8240->hdmi_sink)
				sii8240_check_ckdt_change(sii8240);
#endif
			ret = mhl_read_byte_reg(hdmi, 0xA0, &value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() mhl_read_byte_reg : 0xA0\n", __func__);
				goto err_exit;
			}
			if (value & BIT_TMDS_CSTAT_P3_PDO_MASK) {
				pr_info("sii8240: CKDT: stable\n");
#ifdef SII8240_CHECK_MONITOR
				sii8240->ckdt_stable = true;
#endif
			} else {
				pr_info("sii8240: CKDT: not stable\n");
#ifdef SII8240_CHECK_MONITOR
				sii8240->ckdt_stable = false;
#endif
			}

			clock_stable = true;
		}

		if (int2_status & BIT_INTR5_SCDT_CHANGE) {
			pr_info("sii8240: BIT_INTR5_SCDT_CHANGE\n");
			ret = mhl_read_byte_reg(hdmi, 0xA0, &value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() mhl_read_byte_reg : 0xA0\n", __func__);
				goto err_exit;
			}
			if (BIT_TMDS_CSTAT_P3_SCDT & value) {
				pr_info("BIT_TMDS_CSTAT_P3_SCDT\n");
				memset(&sii8240->aviInfoFrame,
						0x00, INFO_BUFFER);
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
				if (sii8240->hdmi_sink)
					sii8240_check_scdt_reg(sii8240);
#endif
			} else
				pr_info("disable_encryption\n");
		}

		if (int2_status & BIT_INTR5_RPWR5V_CHG) {
			pr_info("sii8240: BIT_INTR5_RPWR5V_CHG\n");
			ret = mhl_read_byte_reg(tmds, 0x81, &value);
			if (unlikely(ret < 0)) {
				pr_err("[ERROR] %s() mhl_read_byte_reg : 0x81\n", __func__);
				goto err_exit;
			}
			if (value & BIT_TMDS_CSTAT_RPWR5V_STATUS)
				pr_info("sii8240: TMDS_CSTAT_RPWR5V_STATUS\n");
		}

		ret = sii8240_cbus_irq(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] sii8240_cbus_irq failed\n");
			goto err_exit;
		}
		ret = sii8240_audio_video_intr_control(sii8240);
		if (unlikely(ret < 0)) {
			pr_err("[ERROR] audio_video_intr_control failed\n");
			goto err_exit;
		}
		break;
	default:
		break;
	}

err_exit:
	/* Clear interrupts */
	ret = mhl_write_byte_reg(disc, DISC_INTR_REG, intr1);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : DISC_INTR_REG\n", __func__);
		goto err_exit2;
	}
	ret = mhl_write_byte_reg(tmds, 0x71, int1_status);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : 0x71\n", __func__);
		goto err_exit2;
	}
	ret = mhl_write_byte_reg(tmds, 0x74, int2_status);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : 0x74\n", __func__);
		goto err_exit2;
	}

	ret = mhl_write_byte_reg(cbus, 0x20, cbus1);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : 0x20\n", __func__);
		goto err_exit2;
	}
	ret = mhl_write_byte_reg(cbus, 0x21, cbus2);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : 0x21\n", __func__);
		goto err_exit2;
	}
	ret = mhl_write_byte_reg(cbus, 0x22, cbus3);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : 0x22\n", __func__);
		goto err_exit2;
	}
	ret = mhl_write_byte_reg(cbus, 0x23, cbus4);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] %s() mhl_write_byte_reg : 0x23\n", __func__);
		goto err_exit2;
	}

err_exit2:
	mutex_unlock(&sii8240->lock);

	wake_up(&sii8240->wq);
	pr_info("sii8240: IRQ_HANDLED\n");
	return IRQ_HANDLED;
}

static int sii8240_mhl_tx_suspend(struct device *dev)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(dev);
	if (sii8240 != NULL) {
		pr_info("%s()\n", __func__);
	/*set config_gpio for mhl*/
		if (likely(sii8240->pdata->gpio_cfg))
			sii8240->pdata->gpio_cfg(MHL_SUSPEND_STATE);
		else
			pr_err("[ERROR] %s() gpio_cfg is NULL\n", __func__);
	}

	return 0;
}

static int sii8240_mhl_tx_resume(struct device *dev)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(dev);
	if (sii8240 != NULL) {
		pr_info("%s()\n", __func__);
	/*set config_gpio for mhl*/
		if (likely(sii8240->pdata->gpio_cfg))
			sii8240->pdata->gpio_cfg(MHL_RESUME_STATE);
		else
			pr_err("[ERROR] %s() gpio_cfg is NULL\n", __func__);
	}

	return 0;
}

static const struct dev_pm_ops sii8240_pm_ops = {
	.suspend        = sii8240_mhl_tx_suspend,
	.resume         = sii8240_mhl_tx_resume,
};


#if defined (CONFIG_MACH_HLTEDCM)
/*
This is only for H-DCM HW REV0.9 and REV1.0.
In those schematics the FELICA I2C pull up voltage source are connected to V_MHL
which is different voltage source from previous HW revision.
This means whenever the FELICA is working the V_MHL is controlled by FELICA driver
and this is why the duplicated extern function is declared and utilized here
even this is kind of prohibited way to make a code.
This has no harmful factor on the other models. This is only for H-DCM HW REV0.9/1.0.
*/
void of_sii8240_hw_poweron(bool enable)
{
	if (g_sii8240 && g_sii8240->pdata && g_sii8240->pdata->power)
		g_sii8240->pdata->power(enable);
	else
		pr_err("[ERROR] %s: some pointer is not initialized (either pdata or pdata->power)\n" , __func__);
}

#endif

#ifdef CONFIG_EXTCON
static int __init sii8240_init_cable_notify(void)
{
	struct sec_mhl_cable *cable;
	int i;
	int ret = 0;

	pr_info("%s register extcon notifier for usb and ta\n", __func__);
	for (i = 0; i < ARRAY_SIZE(support_cable_list); i++) {
		cable = &support_cable_list[i];
		INIT_WORK(&cable->work, sii8240_extcon_work);
		cable->nb.notifier_call = sii8240_extcon_notifier;

		ret = extcon_register_interest(&cable->extcon_nb,
				EXTCON_DEV_NAME,
				extcon_cable_name[cable->cable_type],
				&cable->nb);
		if (ret)
			pr_err("%s: fail to register extcon notifier(%s, %d)\n",
				__func__, extcon_cable_name[cable->cable_type],
				ret);

		cable->edev = cable->extcon_nb.edev;
		if (!cable->edev)
			pr_err("%s: fail to get extcon device\n", __func__);
	}
	return 0;
}
device_initcall_sync(sii8240_init_cable_notify);
#else
static BLOCKING_NOTIFIER_HEAD(acc_mhl_notifier);
void of_sii8240_muic_mhl_notify(int event)
{
	pr_info("%s Attached: %d\n", __func__, event);
	blocking_notifier_call_chain(&acc_mhl_notifier, event, NULL);
}
#endif

static ssize_t sii8240_rda_mhl_version(struct device *dev,
			struct device_attribute *attr,
					char *buf)
{
	ssize_t ret = 0;
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);

	if (((sii8240->regs.peer_devcap[MHL_DEVCAP_MHL_VERSION] & 0xF0) >= 0x20)
		&& (sii8240->regs.peer_devcap[MHL_DEVCAP_VID_LINK_MODE] &
					(MHL_DEV_VID_LINK_SUPP_PPIXEL |
					MHL_DEV_VID_LINK_SUPPYCBCR422))) {
		ret = snprintf(buf, PAGE_SIZE, "%d\n", 2);
	} else
		ret = snprintf(buf, PAGE_SIZE, "%d\n", 1);

	return ret;

}
static DEVICE_ATTR(mhl_version, 0644, sii8240_rda_mhl_version, NULL);

static ssize_t sii8240_hdcp_status(struct class *dev,
		struct class_attribute *attr, char *buf)
{
	int size;

	size = snprintf(buf, 10,"%d", g_monitor_cmd.a);
	return size;
}

static CLASS_ATTR(hdcp_status, 0444,
		sii8240_hdcp_status, NULL);


#if CONFIG_MHL_SWING_LEVEL
static ssize_t sii8240_swing_test_show(struct class *dev,
		struct class_attribute *attr, char *buf)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);

	u32 clk = (sii8240->pdata->swing_level >> 3) & 0x07;
	u32 data = sii8240->pdata->swing_level & 0x07;
	return sprintf(buf, "mhl_show_value:0x%02x(%d%d:Clk=%d,Data=%d)\n"
			, sii8240->pdata->swing_level, clk, data, clk, data);

}
static ssize_t sii8240_swing_test_store(struct class *dev,
		struct class_attribute *attr,
		const char *buf, size_t size)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);

	if (buf[0] >= '0' && buf[0] <= '7' &&
			buf[1] >= '0' && buf[1] <= '7')
		sii8240->pdata->swing_level = ((buf[0] - '0') << 3) |
			(buf[1] - '0');
	else
		sii8240->pdata->swing_level = 0x34; /*Clk=6 and Data=4*/

	return size;
}

static CLASS_ATTR(swing, 0664,
		sii8240_swing_test_show, sii8240_swing_test_store);
extern int hdmi_forced_resolution;
static ssize_t sii8240_timing_test_show(struct class *dev,
		struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", hdmi_forced_resolution);

}

static ssize_t sii8240_timing_test_store(struct class *dev,
		struct class_attribute *attr,
		const char *buf, size_t size)
{
	int timing, ret;

	ret = kstrtouint(buf, 10, &timing);
	if (unlikely(ret < 0))
		return size;

	if (timing >= 0 && timing <= 44)
		hdmi_forced_resolution = timing;
	else
		hdmi_forced_resolution = -1;

	return size;
}

static CLASS_ATTR(timing, 0664,
		sii8240_timing_test_show, sii8240_timing_test_store);

#endif
/* for factory test process */
#ifdef CONFIG_SS_FACTORY
#define SII_ID 0x82
static ssize_t sii8240_test_show(struct class *dev,
				struct class_attribute *attr,
				char *buf)
{
	struct sii8240_data *sii8240 = dev_get_drvdata(sii8240_mhldev);
	struct i2c_client *tmds = sii8240->pdata->tmds_client;
	int size;
	u8 sii_id = 0;

	sii8240->pdata->power(1);
	msleep(20);
	sii8240->pdata->hw_reset();

	mhl_read_byte_reg(tmds, 0x03, &sii_id);
	pr_info("sii8240: check mhl : %X\n", sii_id);

	sii8240->pdata->power(0);

	size = snprintf(buf, 10, "%d\n", sii_id == SII_ID ? 1 : 0);
	return size;
}
static CLASS_ATTR(test_result, 0664, sii8240_test_show, NULL);
#endif

static int __devinit sii8240_tmds_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sii8240_data *sii8240;
	struct kobject *uevent_mhl;
	struct device *mhl_dev;

	dev_info(&client->dev, "success client_addr 0x%X\n", client->addr);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	/* going to use block read/write, so check for this too */
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
		return -EIO;

	sii8240 = kzalloc(sizeof(struct sii8240_data), GFP_KERNEL);
	if (!sii8240) {
		dev_err(&client->dev, "failed to allocate driver data\n");
		return -ENOMEM;
	}

	sii8240->pdata = client->dev.platform_data;
	if (!sii8240->pdata) {
		dev_err(&client->dev, "failed to find platform data\n");
		ret = -EINVAL;
		goto err_exit0;
	}

	sii8240->pdata->tmds_client = client;
	i2c_set_clientdata(client, sii8240);

	mutex_init(&sii8240->lock);
	mutex_init(&sii8240->msc_lock);
	mutex_init(&sii8240->cbus_lock);
	mutex_init(&sii8240->input_lock);

	init_waitqueue_head(&sii8240->wq);

	sii8240->cbus_cmd_wqs = create_workqueue("sii8240-cmdwq");
	if (sii8240->cbus_cmd_wqs == NULL) {
		ret = -ENXIO;
		dev_err(&client->dev, "failed to create cbus_cmd_wqs\n");
		goto err_exit0;
	}

	sii8240->avi_cmd_wqs = create_workqueue("sii8240-aviwq");
	if (sii8240->avi_cmd_wqs == NULL) {
		ret = -ENXIO;
		dev_err(&client->dev, "failed to create avi_cmd_wqs\n");
		goto err_exit0;
	}

	sii8240->mhl_detection_workqueue = create_workqueue("mhl_detection_wq");
	if (!(sii8240->mhl_detection_workqueue)) {
		printk(KERN_ERR
		       "[ERROR] %s() workqueue create fail\n", __func__);
		ret = -ENOMEM;
		goto err_exit0;
	}

	sii8240->mhl_link_monitor_wq = create_workqueue("mhl_link_monitor_wq");
	if (!(sii8240->mhl_link_monitor_wq)) {
		printk(KERN_ERR
		       "[ERROR] %s() workqueue create fail\n", __func__);
		ret = -ENOMEM;
		goto err_exit0;
	}

	/*workqueue for CBUS*/
	INIT_WORK(&sii8240->cbus_work, sii8240_msc_event);
	INIT_LIST_HEAD(&sii8240->cbus_data_list);
	INIT_WORK(&sii8240->redetect_work, sii8240_detection_restart);
	INIT_WORK(&sii8240->avi_control_work, sii8240_avi_control_thread);
#ifdef SII8240_CHECK_MONITOR
	INIT_WORK(&sii8240->mhl_link_monitor_work, sii8240_link_monitor_work);
#endif
#ifdef SFEATURE_UNSTABLE_SOURCE_WA
	INIT_WORK(&sii8240->avi_check_work, sii8240_avi_check_work);
#endif

#ifdef SFEATURE_UNSTABLE_SOURCE_WA
	setup_timer(&sii8240->avi_check_timer, sii8240_avif_check_callback,
				(unsigned long)sii8240);
#endif
#ifdef SII8240_CHECK_MONITOR
	setup_timer(&sii8240->mhl_timer, sii8240_link_monitor_timer, (unsigned long)sii8240);
#endif

	dev_info(&client->dev, "sii8240 irq : %d\n", client->irq);
	sii8240->irq = client->irq;

	sii8240_register_input_device(sii8240);
	sii8240_mhldev = &client->dev;
	g_sii8240 = sii8240;

	ret = request_threaded_irq(client->irq, NULL, sii8240_irq_thread,
			IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
			"sii8240", sii8240);
	if (unlikely(ret < 0)) {
		printk(KERN_ERR "[MHL]request irq Q failing\n");
		goto err_exit0;
	}
	disable_irq(client->irq);
	sii8240->irq_enabled = false;

	wake_lock_init(&sii8240->mhl_wake_lock,
			WAKE_LOCK_SUSPEND, "mhl_wake_lock");

	if (sii8240->pdata->hdmi_mhl_ops) {
		struct msm_hdmi_mhl_ops *hdmi_mhl_ops =	sii8240->pdata->hdmi_mhl_ops;
		hdmi_mhl_ops->set_mhl_max_pclk(sii8240->pdata->hdmi_pdev, 150000);
	}

#ifndef CONFIG_EXTCON
	sii8240->mhl_nb.notifier_call = sii8240_detection_callback;
	acc_register_notifier(&sii8240->mhl_nb);
#endif
	sii8240->mhl_event_switch.name = "mhl_event_switch";
	switch_dev_register(&sii8240->mhl_event_switch);

#ifdef SFEATURE_HDCP_SUPPORT
	sii8240->mhl_ddc_bypass = mhl_ddc_bypass;
#endif
        sec_mhl = class_create(THIS_MODULE, "mhl");
        if (IS_ERR(sec_mhl)) {
                pr_err("[ERROR] Failed to create class(mhl)!\n");
                ret = PTR_ERR(sec_mhl);
                goto err_exit0;
        }

        mhl_dev = device_create(sec_mhl, NULL, MKDEV(MHL_MAJOR, 0),
                                        NULL, "mhl_dev");
        if (IS_ERR(mhl_dev)) {
                pr_err("[ERROR] Failed to create device(mhl_dev)!\n");
                ret = PTR_ERR(mhl_dev);
                goto err_mhl_dev;
        }

        ret = device_create_file(mhl_dev,
                (const struct device_attribute *)&dev_attr_mhl_version.attr);
        if (ret) {
                pr_err("[ERROR] Failed to create device file in sysfs entries(%s)!\n",
                        dev_attr_mhl_version.attr.name);
                goto err_create_file_1;
        }

#ifdef CONFIG_SS_FACTORY
        ret = class_create_file(sec_mhl, &class_attr_test_result);
        if (ret) {
                pr_err("[ERROR] Failed to create device file in sysfs entries!\n");
                goto err_create_file_2;
        }
#endif
#if CONFIG_MHL_SWING_LEVEL
        ret = class_create_file(sec_mhl, &class_attr_swing);
        if (ret) {
                pr_err("[ERROR] failed to create swing sysfs file\n");
                goto err_create_file_3;
        }
        ret = class_create_file(sec_mhl, &class_attr_timing);
        if (ret) {
                pr_err("[ERROR] failed to create timing sysfs file\n");
                goto err_create_file_4;
        }

#endif
        ret = class_create_file(sec_mhl, &class_attr_hdcp_status);
        if (ret) {
                pr_err("[ERROR] failed to create hdcp_status sysfs file\n");
                goto err_create_file_5;
        }
        uevent_mhl = &(mhl_dev->kobj);
	return 0;
err_create_file_5:
#if CONFIG_MHL_SWING_LEVEL
	class_remove_file(sec_mhl, &class_attr_timing);
err_create_file_4:
        class_remove_file(sec_mhl, &class_attr_swing);
err_create_file_3:
#endif
#ifdef CONFIG_SS_FACTORY
        class_remove_file(sec_mhl, &class_attr_test_result);
err_create_file_2:
#endif
        class_remove_file(sec_mhl,
                (const struct class_attribute *)&dev_attr_mhl_version.attr);
err_create_file_1:
        device_destroy(sec_mhl, MKDEV(MHL_MAJOR, 0));
err_mhl_dev:
        class_destroy(sec_mhl);
err_exit0:
	kfree(sii8240);
	return ret;
}

static int __devinit sii8240_hdmi_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	struct sii8240_platform_data *pdata = client->dev.platform_data;

	pdata->hdmi_client = client;
	dev_info(&client->dev, "success client_addr 0x%X\n", client->addr);
	return 0;
}

static int __devinit sii8240_disc_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct sii8240_platform_data *pdata = client->dev.platform_data;

	pdata->disc_client = client;
	dev_info(&client->dev, "success client_addr 0x%X\n", client->addr);
	return 0;
}

static int __devinit sii8240_tpi_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct sii8240_platform_data *pdata = client->dev.platform_data;
	pdata->tpi_client = client;
	dev_info(&client->dev, "success client_addr 0x%X\n", client->addr);
	return 0;
}

static int __devinit sii8240_cbus_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct sii8240_platform_data *pdata = client->dev.platform_data;
	pdata->cbus_client = client;
	dev_info(&client->dev, "success client_addr 0x%X\n", client->addr);
	return 0;
}

#ifdef CONFIG_OF
static int __devinit of_sii8240_probe_dt(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	const struct i2c_device_id *id_table = client->driver->id_table;
	struct sii8240_platform_data *pdata = NULL;
	u32 client_id = -1;

	if (!client->dev.of_node) {
		dev_err(&client->dev, "sii8240: Client node not-found\n");
		return -1;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	/* going to use block read/write, so check for this too */
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
		return -EIO;

	if (of_property_read_u32(client->dev.of_node, "sii8240,client_id", &client_id))
		dev_err(&client->dev, "Wrong Client_id# %d", client_id);

	if (0 == client_id) {
		pdata = platform_init_data(client);
		client->dev.platform_data = pdata;
		sii8240_tmds_i2c_probe(client, id_table);
	} else if (g_sii8240) {
		client->dev.platform_data = g_sii8240->pdata;
		if (1 == client_id)
			sii8240_hdmi_i2c_probe(client, id_table);
		else if (2 == client_id)
			sii8240_disc_i2c_probe(client, id_table);
		else if (3 == client_id)
			sii8240_tpi_i2c_probe(client, id_table);
		else if (4 == client_id)
			sii8240_cbus_i2c_probe(client, id_table);
	 }

	return 0;
}
#endif

static int __devexit sii8240_tmds_remove(struct i2c_client *client)
{
	return 0;
}

static int __devexit sii8240_hdmi_remove(struct i2c_client *client)
{
	return 0;
}

static int __devexit sii8240_disc_remove(struct i2c_client *client)
{
	return 0;
}

static int __devexit sii8240_tpi_remove(struct i2c_client *client)
{
	return 0;
}

static int __devexit sii8240_cbus_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id sii8240_tmds_id[] = {
	{"sii8240_tmds", 0},
	{}
};
static const struct i2c_device_id sii8240_hdmi_id[] = {
	{"sii8240_hdmi", 0},
	{}
};
static const struct i2c_device_id sii8240_disc_id[] = {
	{"sii8240_disc", 0},
	{}
};
static const struct i2c_device_id sii8240_tpi_id[] = {
	{"sii8240_tpi", 0},
	{}
};
static const struct i2c_device_id sii8240_cbus_id[] = {
	{"sii8240_cbus", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sii8240_tmds_id);
MODULE_DEVICE_TABLE(i2c, sii8240_hdmi_id);
MODULE_DEVICE_TABLE(i2c, sii8240_disc_id);
MODULE_DEVICE_TABLE(i2c, sii8240_tpi_id);
MODULE_DEVICE_TABLE(i2c, sii8240_cbus_id);

#ifdef CONFIG_OF
static struct of_device_id sii8240_dt_ids[] = {
	{ .compatible = "sii8240,tmds",},
	{ .compatible = "sii8240,hdmi",},
	{ .compatible = "sii8240,disc",},
	{ .compatible = "sii8240,tpi",},
	{ .compatible = "sii8240,cbus",},
	{}
};
MODULE_DEVICE_TABLE(of, sii8240_dt_ids);

static const struct i2c_device_id sii8240_id[] = {
	{"sii8240_mhlv2", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sii8240_id);

static struct i2c_driver sii8240_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sii8240_mhlv2dt",
		.of_match_table = of_match_ptr(sii8240_dt_ids),
		.pm = &sii8240_pm_ops,
	},
	.id_table = sii8240_id,
	.probe = of_sii8240_probe_dt,
};
#else

static struct i2c_driver sii8240_tmds_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sii8240_tmds",
		.pm = &sii8240_pm_ops,
	},
	.id_table = sii8240_tmds_id,
	.probe = sii8240_tmds_i2c_probe,
	.remove = __devexit_p(sii8240_tmds_remove),
};
static struct i2c_driver sii8240_hdmi_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sii8240_hdmi",
	},
	.id_table = sii8240_hdmi_id,
	.probe = sii8240_hdmi_i2c_probe,
	.remove = __devexit_p(sii8240_hdmi_remove),
};
static struct i2c_driver sii8240_disc_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sii8240_disc",
	},
	.id_table = sii8240_disc_id,
	.probe = sii8240_disc_i2c_probe,
	.remove = __devexit_p(sii8240_disc_remove),
};
static struct i2c_driver sii8240_tpi_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sii8240_tpi",
	},
	.id_table = sii8240_tpi_id,
	.probe = sii8240_tpi_i2c_probe,
	.remove = __devexit_p(sii8240_tpi_remove),
};
static struct i2c_driver sii8240_cbus_i2c_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "sii8240_cbus",
	},
	.id_table = sii8240_cbus_id,
	.probe = sii8240_cbus_i2c_probe,
	.remove = __devexit_p(sii8240_cbus_remove),
};
#endif

static int __init sii8240_init(void)
{
	int ret;

	pr_info("%s sii8240: check mhl\n", __func__);
#ifdef CONFIG_OF
	ret = i2c_add_driver(&sii8240_i2c_driver);
	if (ret < 0) {
		pr_err("[ERROR] sii8240: mhl_v2 i2c driver init failed");
		return ret;
	}
#else
	ret = i2c_add_driver(&sii8240_tmds_i2c_driver);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: tmds i2c driver init failed");
		goto err_tmds;
	}

	ret = i2c_add_driver(&sii8240_hdmi_i2c_driver);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: hdmi i2c driver init failed");
		goto err_hdmi;
	}

	ret = i2c_add_driver(&sii8240_disc_i2c_driver);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: disc i2c driver init failed");
		goto err_disc;
	}

	ret = i2c_add_driver(&sii8240_tpi_i2c_driver);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: tpi i2c driver init failed");
		goto err_tpi;
	}

	ret = i2c_add_driver(&sii8240_cbus_i2c_driver);
	if (unlikely(ret < 0)) {
		pr_err("[ERROR] sii8240: cbus i2c driver init failed");
		goto err_cbus;
	}
#endif
	return 0;

#ifndef CONFIG_OF
err_cbus:
	i2c_del_driver(&sii8240_cbus_i2c_driver);
err_tpi:
	i2c_del_driver(&sii8240_tpi_i2c_driver);
err_disc:
	i2c_del_driver(&sii8240_disc_i2c_driver);
err_hdmi:
	i2c_del_driver(&sii8240_hdmi_i2c_driver);
err_tmds:
	i2c_del_driver(&sii8240_tmds_i2c_driver);
#endif
#ifdef CONFIG_OF
	i2c_del_driver(&sii8240_i2c_driver);
#endif
	return ret;
}
static void __exit sii8240_exit(void)
{
#ifdef CONFIG_OF
	i2c_del_driver(&sii8240_i2c_driver);
#else
	i2c_del_driver(&sii8240_cbus_i2c_driver);
	i2c_del_driver(&sii8240_tpi_i2c_driver);
	i2c_del_driver(&sii8240_disc_i2c_driver);
	i2c_del_driver(&sii8240_hdmi_i2c_driver);
	i2c_del_driver(&sii8240_tmds_i2c_driver);
#endif
}
module_init(sii8240_init);
module_exit(sii8240_exit);
MODULE_DESCRIPTION("Silicon Image MHL-8240 Transmitter driver");
MODULE_AUTHOR("Sangmi Park <sm0327.park@samsung.com>");
MODULE_LICENSE("GPL");
