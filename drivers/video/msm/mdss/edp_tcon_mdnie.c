/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fb.h>
#include <linux/msm_mdp.h>
#include <linux/ioctl.h>
#include <linux/lcd.h>

#if defined(CONFIG_SEC_LT03_PROJECT)
#include "n1_edp_mdnie_data.h"
#elif defined(CONFIG_SEC_PICASSO_PROJECT)
#include "picasso_edp_mdnie_data.h"
#else //defined(CONFIG_SEC_VIENNA_PROJECT) || defined(CONFIG_SEC_V2_PROJECT)
#include "v1_edp_mdnie_data.h"
#endif

#define MDNIE_LITE_TUN_DEBUG

#ifdef MDNIE_LITE_TUN_DEBUG
#define DPRINT(x...)	printk(KERN_ERR "[mdnie lite] " x)
#else
#define DPRINT(x...)
#endif

static struct class *mdnie_class;

struct device *tune_mdnie_dev;

static struct mdnie_lite_tun_type mdnie_tun_state = {
	.mdnie_enable = false,
	.mdnie_bypass = BYPASS_DISABLE,
	.cabc_bypass = BYPASS_DISABLE,
	.mdnie_app = UI_APP,
	.mdnie_mode = STANDARD_MODE,
	.mdnie_accessibility = ACCESSIBILITY_OFF,
};

int update_mdnie_register(void)
{
	short *tune_data = NULL;

	if (mdnie_tun_state.mdnie_bypass == BYPASS_ENABLE)
		tune_data = BYPASS_MDNIE;
	else if (mdnie_tun_state.mdnie_accessibility == NEGATIVE)
		tune_data = NEGATIVE_MDNIE;
	else if (mdnie_tun_state.mdnie_accessibility == COLOR_BLIND)
		tune_data = COLOR_BLIND_MDNIE;
	else
		tune_data = mdnie_tune_value[mdnie_tun_state.mdnie_app][mdnie_tun_state.mdnie_mode];

	if (!tune_data) {
		DPRINT("%s tune_data is NULL mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d\n", __func__,
			mdnie_tun_state.mdnie_bypass, mdnie_tun_state.mdnie_accessibility,
			mdnie_tun_state.mdnie_app, mdnie_tun_state.mdnie_mode);
		return -EFAULT;
	} else {
		DPRINT("%s mdnie_bypass : %d mdnie_accessibility : %d  mdnie_app: %d mdnie_mode : %d\n", __func__,
			mdnie_tun_state.mdnie_bypass, mdnie_tun_state.mdnie_accessibility,
			mdnie_tun_state.mdnie_app, mdnie_tun_state.mdnie_mode);
	}

	mdnie_tune_cmd(tune_data , MDNIE_COL);
	return 0;
}

static ssize_t mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	DPRINT("%s mode : %s\n", __func__, mdnie_app_name[mdnie_tun_state.mdnie_mode]);
	return snprintf(buf, 256, "Current Mode : %s\n",
		mdnie_mode_name[mdnie_tun_state.mdnie_mode]);
}

static ssize_t mode_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	if (value < DYNAMIC_MODE || value >= MAX_MODE) {
		DPRINT("[ERROR] wrong mode value : %d\n",
			value);
		return size;
	}

	mdnie_tun_state.mdnie_mode = value;

	DPRINT("%s mode : %d\n", __func__, mdnie_tun_state.mdnie_mode);
	update_mdnie_register();
	return size;
}
static DEVICE_ATTR(mode, 0664, mode_show, mode_store);

static ssize_t scenario_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	DPRINT("%s APP : %s\n", __func__, mdnie_app_name[mdnie_tun_state.mdnie_app]);

	return snprintf(buf, 256, "Current APP : %s\n",
		mdnie_app_name[mdnie_tun_state.mdnie_app]);
}

static ssize_t scenario_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);
	if (value < UI_APP || value >= MAX_APP_MODE) {
		DPRINT("[ERROR] wrong Scenario mode value : %d\n",
			value);
		return size;
	}

	mdnie_tun_state.mdnie_app = value;

	DPRINT("%s APP : %d\n", __func__, mdnie_tun_state.mdnie_app);
	update_mdnie_register();
	return size;
}
static DEVICE_ATTR(scenario, 0664, scenario_show,
		   scenario_store);

static ssize_t bypass_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	DPRINT("%s bypass : %s\n", __func__, mdnie_tun_state.mdnie_bypass ? "ENABLE" : "DISABLE");

	return snprintf(buf, 256, "Current MDNIE bypass : %s\n",
		mdnie_tun_state.mdnie_bypass ? "ENABLE" : "DISABLE");
}

static ssize_t bypass_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);
	
	if (value)
		mdnie_tun_state.mdnie_bypass = BYPASS_ENABLE;
	else 
		mdnie_tun_state.mdnie_bypass = BYPASS_DISABLE;

	DPRINT("%s bypass : %s value : %d\n", __func__, mdnie_tun_state.mdnie_bypass ? "ENABLE" : "DISABLE", value);

	update_mdnie_register();

	return size;
}
static DEVICE_ATTR(bypass, 0664, bypass_show,
		   bypass_store);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
extern int config_cabc(int power_save_mode);

static ssize_t cabc_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	DPRINT("%s bypass : %s\n", __func__, mdnie_tun_state.cabc_bypass ? "ENABLE" : "DISABLE");

	return snprintf(buf, 256, "Current CABC bypass : %s\n",
		mdnie_tun_state.cabc_bypass ? "ENABLE" : "DISABLE");
}

static ssize_t cabc_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	int value;

	sscanf(buf, "%d", &value);

	if (value)
		mdnie_tun_state.cabc_bypass = BYPASS_ENABLE; /* ONLY POWER SAVE MODE */
	else 
		mdnie_tun_state.cabc_bypass = BYPASS_DISABLE; /* DBLC MODE */

	DPRINT("%s bypass : %s value : %d\n", __func__, mdnie_tun_state.cabc_bypass ? "ENABLE" : "DISABLE", value);

	config_cabc(mdnie_tun_state.cabc_bypass);

	return size;
}
static DEVICE_ATTR(cabc, 0664, cabc_show, cabc_store);
#endif

#if 0
static ssize_t negative_show(struct device *dev,
					      struct device_attribute *attr,
					      char *buf)
{
	DPRINT("%s %s\n", __func__, (mdnie_tun_state.mdnie_negative == 0) ? "Disabled" : "Enabled");
	return snprintf(buf, 256, "Current negative Value : %s\n",
		(mdnie_tun_state.mdnie_negative == 0) ? "Disabled" : "Enabled");
}
static ssize_t negative_store(struct device *dev,
					       struct device_attribute *attr,
					       const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);
	mdnie_tun_state.mdnie_negative = value;
	DPRINT("%s value = %d\n", __func__, mdnie_tun_state.mdnie_negative);
	update_mdnie_register();	
	return size;
}
static DEVICE_ATTR(negative, 0664,
		   negative_show,
		   negative_store);
#endif
static ssize_t accessibility_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	DPRINT("%s %s\n", __func__, mdnie_tun_state.mdnie_accessibility ? 
		mdnie_tun_state.mdnie_accessibility == 1 ? "NEGATIVE" : "COLOR_BLIND" : "ACCESSIBILITY_OFF");
	return snprintf(buf, 256, "%s %s\n", __func__, mdnie_tun_state.mdnie_accessibility ? 
		mdnie_tun_state.mdnie_accessibility == 1 ? "NEGATIVE" : "COLOR_BLIND" : "ACCESSIBILITY_OFF");
}

static ssize_t accessibility_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t size)
{
	int cmd_value;
	char buffer[MDNIE_COLOR_BLINDE_CMD_SIZE] = {0,};
	int buffer2[MDNIE_COLOR_BLINDE_CMD_SIZE/2] = {0,};
	int loop;
	char temp;

	sscanf(buf, "%d %x %x %x %x %x %x %x %x %x", &cmd_value,
		&buffer2[0], &buffer2[1], &buffer2[2], &buffer2[3], &buffer2[4],
		&buffer2[5], &buffer2[6], &buffer2[7], &buffer2[8]);

	for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD_SIZE/2; loop++) {
		buffer2[loop] = buffer2[loop] & 0xFFFF;
		buffer[loop * 2] = (buffer2[loop] & 0xFF00) >> 8;
		buffer[loop * 2 + 1] = buffer2[loop] & 0xFF;
	}

	for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD_SIZE; loop+=2) {
		temp = buffer[loop];
		buffer[loop] = buffer[loop + 1];
		buffer[loop + 1] = temp;
	}

	if (cmd_value == NEGATIVE)
		mdnie_tun_state.mdnie_accessibility = NEGATIVE;
	else if (cmd_value == COLOR_BLIND) {
		mdnie_tun_state.mdnie_accessibility = COLOR_BLIND;
		for(loop = 0; loop < MDNIE_COLOR_BLINDE_CMD_SIZE; loop++)
			COLOR_BLIND_MDNIE[(MDNIE_ROW * MDNIE_COLOR_BLINDE_CM_START) + (loop *MDNIE_ROW) + 1] = buffer[loop];
	} else if (cmd_value == ACCESSIBILITY_OFF)
		mdnie_tun_state.mdnie_accessibility = ACCESSIBILITY_OFF;
	else
		pr_info("%s ACCESSIBILITY_MAX", __func__);

	pr_info("%s cmd_value : %d size : %d", __func__, cmd_value, size);

	update_mdnie_register();
	return size;
}
static DEVICE_ATTR(accessibility, 0664, accessibility_show,
		   accessibility_store);

void init_mdnie_class(void)
{
	mdnie_class = class_create(THIS_MODULE, "mdnie");
	if (IS_ERR(mdnie_class))
		pr_err("Failed to create class(mdnie)!\n");
	tune_mdnie_dev =
	    device_create(mdnie_class, NULL, 0, NULL,
		  "mdnie");
	if (IS_ERR(tune_mdnie_dev))
		pr_err("Failed to create device(mdnie)!\n");
	/* APP */
	if (device_create_file
	    (tune_mdnie_dev, &dev_attr_scenario) < 0)
		pr_err("Failed to create device file(%s)!\n",
	       dev_attr_scenario.attr.name);
	/* MODE */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_mode) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_mode.attr.name);

	/* MDNIE ON/OFF */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_bypass) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_bypass.attr.name);
#if 0
	/* NEGATIVE */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_negative) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_negative.attr.name);
#endif
	/* COLOR BLIND */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_accessibility) < 0)
		pr_err("Failed to create device file(%s)!=n",
			dev_attr_accessibility.attr.name);

#if defined(CONFIG_FB_MSM_EDP_SAMSUNG)
	/* CABC ON/OFF */
	if (device_create_file
		(tune_mdnie_dev, &dev_attr_cabc) < 0)
		pr_err("Failed to create device file(%s)!\n",
			dev_attr_cabc.attr.name);
#endif
	mdnie_tun_state.mdnie_enable = true;
}
