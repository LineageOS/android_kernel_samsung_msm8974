/* Synaptics Register Mapped Interface (RMI4) I2C Physical Layer Driver.
 * Copyright (c) 2007-2012, Synaptics Incorporated
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/unaligned.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include "synaptics_i2c_rmi.h"

#include <linux/i2c/synaptics_rmi.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/qpnp/pin.h>

#define DRIVER_NAME "synaptics_rmi4_i2c"

#define SYNAPTICS_PM_GPIO_STATE_WAKE	0
#define SYNAPTICS_PM_GPIO_STATE_SLEEP	1

struct qpnp_pin_cfg synaptics_int_set[] = {
	{
		.mode = 0,
		.pull = 5,
		.output_type = 0,
		.invert = 0,
		.vin_sel = 2,
		.out_strength = 2,
		.src_sel = 0,
		.master_en = 1,
	},
	{
		.mode = 0,
		.pull = 4,
		.output_type = 0,
		.invert = 0,
		.vin_sel = 2,
		.out_strength = 2,
		.src_sel = 0,
		.master_en = 1,
	},
};

struct list_head exp_fn_list;

void synaptics_power_ctrl(struct synaptics_rmi4_data *rmi4_data, bool enable);

static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);

static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data,
		unsigned short length);

static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_stop_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_start_device(struct synaptics_rmi4_data *rmi4_data);
static int synaptics_rmi4_init_exp_fn(struct synaptics_rmi4_data *rmi4_data);
static void synaptics_rmi4_remove_exp_fn(struct synaptics_rmi4_data *rmi4_data);

#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static void synaptics_rmi4_early_suspend(struct early_suspend *h);

static void synaptics_rmi4_late_resume(struct early_suspend *h);

#else

static int synaptics_rmi4_suspend(struct device *dev);

static int synaptics_rmi4_resume(struct device *dev);
#endif

#ifdef PROXIMITY
static void synaptics_rmi4_f51_finger_timer(unsigned long data);

static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f51_general_control_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f51_general_control_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
#endif

#ifdef USE_OPEN_CLOSE
static int synaptics_rmi4_input_open(struct input_dev *dev);
static void synaptics_rmi4_input_close(struct input_dev *dev);
#endif

static ssize_t synaptics_rmi4_register_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_global_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_global_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

#ifdef GLOVE_MODE
static ssize_t synaptics_rmi4_glove_mode_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_glove_mode_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_closed_cover_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_closed_cover_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_fast_detect_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_fast_detect_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static ssize_t synaptics_rmi4_suspend_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

static struct device_attribute attrs[] = {
	__ATTR(regval, (S_IRUGO | S_IWUSR | S_IWGRP),
			NULL,
			synaptics_rmi4_register_value_store),
	__ATTR(global, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_global_show,
			synaptics_rmi4_global_store),
#ifdef CONFIG_HAS_EARLYSUSPEND
	__ATTR(full_pm_cycle, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_full_pm_cycle_show,
			synaptics_rmi4_full_pm_cycle_store),
#endif
#ifdef PROXIMITY
	__ATTR(proximity_enables, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_f51_enables_show,
			synaptics_rmi4_f51_enables_store),
	__ATTR(general_control, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_f51_general_control_show,
			synaptics_rmi4_f51_general_control_store),
#endif
#ifdef GLOVE_MODE
	__ATTR(glove_mode_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_glove_mode_enable_show,
			synaptics_rmi4_glove_mode_enable_store),
	__ATTR(closed_cover_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_closed_cover_enable_show,
			synaptics_rmi4_closed_cover_enable_store),
	__ATTR(fast_detect_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_fast_detect_enable_show,
			synaptics_rmi4_fast_detect_enable_store),
#endif
	__ATTR(reset, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			synaptics_rmi4_f01_reset_store),
	__ATTR(productinfo, S_IRUGO,
			synaptics_rmi4_f01_productinfo_show,
			synaptics_rmi4_store_error),
	__ATTR(buildid, S_IRUGO,
			synaptics_rmi4_f01_buildid_show,
			synaptics_rmi4_store_error),
	__ATTR(flashprog, S_IRUGO,
			synaptics_rmi4_f01_flashprog_show,
			synaptics_rmi4_store_error),
	__ATTR(0dbutton, (S_IRUGO | S_IWUSR | S_IWGRP),
			synaptics_rmi4_0dbutton_show,
			synaptics_rmi4_0dbutton_store),
	__ATTR(suspend, S_IWUSR | S_IWGRP,
			synaptics_rmi4_show_error,
			synaptics_rmi4_suspend_store),
};

#ifdef READ_LCD_ID
static int synaptics_lcd_id;
static int __init synaptics_read_lcd_id(char *mode)
{
	int retval = 0, count = 0;

	if (mode == NULL)
		return -EINVAL;

	while ((mode[count] != (int)NULL) && (mode[count] >= '0') && \
			(mode[count] <= 'z')) {
		retval = retval * 0x10 + mode[count] - '0';
		++count;
	}

	synaptics_lcd_id = ((retval >> 8) & 0xFF) >> 4;
	printk(KERN_ERR "%s: string is %s, integer is %x, ID2[0x%02x], system_rev is %d\n",
			__func__, mode, retval, synaptics_lcd_id, system_rev);

	return 0;
}
__setup("lcd_id=0x", synaptics_read_lcd_id);
#endif

#ifdef CONFIG_OF
static int synaptics_parse_dt(struct device *dev,
		struct synaptics_rmi4_device_tree_data *dt_data)
{
	struct device_node *np = dev->of_node;
	struct property *prop;
	struct synaptics_rmi_f1a_button_map *f1a_button_map;
	int rc;
	int i;
	u32 coords[2];

	/* vdd, irq gpio info */
	dt_data->external_ldo = of_get_named_gpio(np, "synaptics,external_ldo", 0);
#if defined(CONFIG_SEC_RUBENS_PROJECT)
	dt_data->external_ldo2 = of_get_named_gpio(np, "synaptics,external_ldo2", 0);
#endif
	dt_data->scl_gpio = of_get_named_gpio(np, "synaptics,scl-gpio", 0);
	dt_data->sda_gpio = of_get_named_gpio(np, "synaptics,sda-gpio", 0);
	dt_data->irq_gpio = of_get_named_gpio(np, "synaptics,irq-gpio", 0);
	dt_data->reset_gpio = of_get_named_gpio(np, "synaptics,reset-gpio", 0);
	dt_data->id_gpio = of_get_named_gpio(np, "synaptics,id-gpio", 0);
	dt_data->tablet = of_property_read_bool(np, "synaptics,tablet");

	rc = of_property_read_u32_array(np, "synaptics,tsp-coords", coords, 2);
	if (rc < 0) {
		dev_info(dev, "%s: Unable to read synaptics,tsp-coords\n", __func__);
		return rc;
	}

	dt_data->coords[0] = coords[0];
	dt_data->coords[1] = coords[1];

	rc = of_property_read_u32(np, "synaptics,supply-num", &dt_data->num_of_supply);
	if (dt_data->num_of_supply > 0) {
		dt_data->name_of_supply = kzalloc(sizeof(char *) * dt_data->num_of_supply, GFP_KERNEL);
		for (i = 0; i < dt_data->num_of_supply; i++) {
			rc = of_property_read_string_index(np, "synaptics,supply-name",
				i, &dt_data->name_of_supply[i]);
			if (rc && (rc != -EINVAL)) {
				dev_err(dev, "%s: Unable to read %s\n", __func__,
						"synaptics,supply-name");
				return rc;
			}
			dev_info(dev, "%s: supply%d: %s\n", __func__, i, dt_data->name_of_supply[i]);
		}
	}

	/* extra config value
	 * @config[0] : Can set pmic regulator voltage.
	 * @config[1][2][3] is not fixed.
	 */
	rc = of_property_read_u32_array(np, "synaptics,tsp-extra_config", dt_data->extra_config, 4);
	if (rc < 0)
		dev_info(dev, "%s: Unable to read synaptics,tsp-extra_config\n", __func__);

	/* f1a button info, touchkey LED */
	dt_data->tkey_led_en = of_get_named_gpio(np, "synaptics,tkeyled-gpio", 0);

	prop = of_find_property(np, "synaptics,tsp-keycodes", NULL);
	if (prop && prop->value) {
		f1a_button_map = kzalloc(sizeof(*f1a_button_map), GFP_KERNEL);
		if (!f1a_button_map) {
			dev_err(dev, "Failed to allocate f1a memory\n");
			return -ENOMEM;
		}

		f1a_button_map->nbuttons = prop->length / sizeof(u32);

		rc = of_property_read_u32_array(np, "synaptics,tsp-keycodes",
			f1a_button_map->map, f1a_button_map->nbuttons);
		if (rc && (rc != -EINVAL)) {
			dev_info(dev, "%s: Unable to read %s, free button map memory\n", __func__,
					"synaptics,tsp-keycodes");
			kfree(f1a_button_map);
			return rc;
		}

		dt_data->f1a_button_map = f1a_button_map;

		pr_err("%s tkey enabled! ", __func__);
		for (i = 0; i < dt_data->f1a_button_map->nbuttons; i++)
			pr_cont("keycode[%d] = %d ", i,
				dt_data->f1a_button_map->map[i]);
		pr_cont("\n");
	}

	/* project info */
	rc = of_property_read_string(np, "synaptics,tsp-project", &dt_data->project);
	if (rc < 0) {
		dev_info(dev, "%s: Unable to read synaptics,tsp-project\n", __func__);
		dt_data->project = "0";
	}
	rc = of_property_read_string(np, "synaptics,sub-project", &dt_data->sub_project);
	if (rc < 0) {
		dev_info(dev, "%s: Unable to read synaptics,sub-project\n", __func__);
		dt_data->sub_project = "0";
	}

	if (dt_data->extra_config[2] > 0) {
		pr_err("%s: OCTA ID = %d\n", __func__,
				gpio_get_value(dt_data->extra_config[2]));

		if (strncmp(dt_data->project, "PSLTE", 5) == 0)
			gpio_tlmm_config(GPIO_CFG(dt_data->extra_config[2], 0,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,
					GPIO_CFG_2MA), 1);

		if ((strncmp(dt_data->project, "K", 1) == 0) &&
				(strncmp(dt_data->sub_project, "active", 6) == 0))
			gpio_tlmm_config(GPIO_CFG(dt_data->extra_config[2], 0,
					GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN,
					GPIO_CFG_2MA), 1);
	}

	dt_data->surface_only = of_property_read_bool(np, "synaptics,surface-only");

	pr_err("%s: power= %d, tsp_int= %d, X= %d, Y= %d, project= %s, "
		"config[%d][%d][%d][%d], tablet = %d reset= %d surface-only= %d\n",
		__func__, dt_data->external_ldo, dt_data->irq_gpio,
			dt_data->coords[0], dt_data->coords[1], dt_data->project,
			dt_data->extra_config[0], dt_data->extra_config[1], dt_data->extra_config[2],
			dt_data->extra_config[3], dt_data->tablet, dt_data->reset_gpio,
			dt_data->surface_only);

	return 0;
}
#else
static int synaptics_parse_dt(struct device *dev,
		struct synaptics_rmi4_device_tree_data *dt_data)
{
	return -ENODEV;
}
#endif

static void synaptics_request_gpio(struct synaptics_rmi4_data *rmi4_data)
{
	int ret;

	ret = gpio_request(rmi4_data->dt_data->irq_gpio, "synaptics,irq_gpio");
	if (ret) {
		pr_err("%s: unable to request irq_gpio [%d]\n",
				__func__, rmi4_data->dt_data->irq_gpio);
		return;
	}

	if (rmi4_data->dt_data->external_ldo > 0) {
		ret = gpio_request(rmi4_data->dt_data->external_ldo, "synaptics,external_ldo");
		if (ret) {
			pr_err("%s: unable to request external_ldo [%d]\n",
					__func__, rmi4_data->dt_data->external_ldo);
			return;
		}
	}
#if defined(CONFIG_SEC_RUBENS_PROJECT)
	if (rmi4_data->dt_data->external_ldo2 > 0) {
		ret = gpio_request(rmi4_data->dt_data->external_ldo2, "synaptics,external_ldo2");
		if (ret) {
			pr_err("%s: unable to request external_ldo [%d]\n",
					__func__, rmi4_data->dt_data->external_ldo2);
			return;
		}
	}
#endif
	if (rmi4_data->dt_data->tkey_led_en > 0) {
		ret = gpio_request(rmi4_data->dt_data->tkey_led_en, "synaptics,tkey_led_vdd_on");
		if (ret) {
			pr_err("%s: unable to request tkey_led_vdd_on [%d]\n",
					__func__, rmi4_data->dt_data->tkey_led_en);
			return;
		}
	}

	if (rmi4_data->dt_data->reset_gpio > 0) {
		ret = gpio_request(rmi4_data->dt_data->reset_gpio, "synaptics,reset-gpio");
		if (ret) {
			pr_err("%s: unable to request reset_gpio [%d]\n",
					__func__, rmi4_data->dt_data->reset_gpio);
			return;
		}
	}
	gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->scl_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->sda_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

	if (rmi4_data->dt_data->id_gpio > 0) {
		ret = gpio_request(rmi4_data->dt_data->id_gpio, "synaptics,id-gpio");
		if (ret) {
			pr_err("%s: unable to request id_gpio [%d]\n",
					__func__, rmi4_data->dt_data->id_gpio);
			return;
		}
	}
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static ssize_t synaptics_rmi4_full_pm_cycle_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->full_pm_cycle);
}

static ssize_t synaptics_rmi4_full_pm_cycle_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	rmi4_data->full_pm_cycle = input > 0 ? 1 : 0;

	return count;
}
#endif

#ifdef PROXIMITY
static ssize_t synaptics_rmi4_f51_enables_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->f51_handle)
		return -ENODEV;

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			rmi4_data->f51_handle->proximity_enables);
}

static ssize_t synaptics_rmi4_f51_enables_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->f51_handle)
		return -ENODEV;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	rmi4_data->f51_handle->proximity_enables = (unsigned char)input;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->proximity_enables_addr,
			&rmi4_data->f51_handle->proximity_enables,
			sizeof(rmi4_data->f51_handle->proximity_enables));
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to write proximity enables, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f51_general_control_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	int retval;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->f51_handle)
		return -ENODEV;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->general_control_addr,
			&data,
			sizeof(data));
	if (retval < 0)
		dev_err(dev,
				"%s: Failed to read general control, error = %d\n",
				__func__, retval);

	return snprintf(buf, PAGE_SIZE, "0x%02x\n",
			data);
}

static ssize_t synaptics_rmi4_f51_general_control_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->f51_handle)
		return -ENODEV;

	if (sscanf(buf, "%x", &input) != 1)
		return -EINVAL;

	rmi4_data->f51_handle->general_control = (unsigned char)input;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->general_control_addr,
			&rmi4_data->f51_handle->general_control,
			sizeof(rmi4_data->f51_handle->general_control));
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to write general control, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}
#endif

static ssize_t synaptics_rmi4_register_value_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input, i;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	unsigned char data[10];
	unsigned int retval;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			input,
			&data[0],
			sizeof(data));

	for (i = 0; i < 10; i++)
		dev_info(&rmi4_data->i2c_client->dev, "%s:[%d] register address=0x%X, register value= %X\n",
				__func__, i, input, data[i]);

	return count;
}

static ssize_t synaptics_rmi4_global_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int retval;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	unsigned char data;

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			0x0c,
			&data,
			sizeof(data));
	dev_info(&rmi4_data->i2c_client->dev, "%s: device_control: %x\n[%d]",
			__func__, data, retval);
	data = 0;
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			0x0d,
			&data,
			sizeof(data));
	dev_info(&rmi4_data->i2c_client->dev, "%s: device_status: %x\n[%d]",
			__func__, data, retval);
	return snprintf(buf, PAGE_SIZE, "%u\n",
			retval);
}

static ssize_t synaptics_rmi4_global_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	unsigned char data[8];
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			0x06,
			&data[0],
			sizeof(data));
	dev_info(&rmi4_data->i2c_client->dev, "%s: %x, %x, %x, %x, %x, %x, %x, %x\n",
			__func__, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);

	return count;
}

#ifdef GLOVE_MODE
#ifdef ENABLE_F12_OBJTYPE
static int synaptics_rmi4_f12_obj_type_enable(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f12_ctrl23_addr,
			&rmi4_data->obj_type_enable,
			sizeof(rmi4_data->obj_type_enable));
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev, "%s: write fail[%d]\n",
			__func__, retval);

	return retval;
}
#endif

int synaptics_rmi4_glove_mode_enables(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;

	if (rmi4_data->touch_stopped)
		goto out;

	retval = synaptics_rmi4_f12_set_feature(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: f12 set_feature write fail[%d]\n", __func__, retval);
	}
	
#ifdef ENABLE_F12_OBJTYPE
	retval = synaptics_rmi4_f12_obj_type_enable(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: f12 obj_type write fail[%d]\n", __func__, retval);
	}
#endif
out:
	return retval;
}

static ssize_t synaptics_rmi4_glove_mode_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int output;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (rmi4_data->feature_enable & GLOVE_MODE_EN)
		output = 1;
	else
		output = 0;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			output);
}

static ssize_t synaptics_rmi4_glove_mode_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	rmi4_data->feature_enable &= ~(GLOVE_MODE_EN);
	if (input)
		rmi4_data->feature_enable |= GLOVE_MODE_EN;

	retval = synaptics_rmi4_glove_mode_enables(rmi4_data);
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to set glove mode enable, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_closed_cover_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int output;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (rmi4_data->feature_enable & CLOSED_COVER_EN)
		output = 1;
	else
		output = 0;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			output);
}

static ssize_t synaptics_rmi4_closed_cover_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	rmi4_data->feature_enable &= ~(CLOSED_COVER_EN);
	if (input)
		rmi4_data->feature_enable |= CLOSED_COVER_EN;

	retval = synaptics_rmi4_f12_set_feature(rmi4_data);
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to set closed cover enable, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_fast_detect_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int output;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (rmi4_data->feature_enable & FAST_DETECT_EN)
		output = 1;
	else
		output = 0;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			output);
}

static ssize_t synaptics_rmi4_fast_detect_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (!rmi4_data->has_glove_mode)
		return -ENODEV;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	rmi4_data->feature_enable &= ~(FAST_DETECT_EN);
	if (input)
		rmi4_data->feature_enable |= FAST_DETECT_EN;

	retval = synaptics_rmi4_f12_set_feature(rmi4_data);
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to set fast detect enable, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}
#endif

#ifdef TSP_BOOSTER
static void synaptics_change_dvfs_lock(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
				struct synaptics_rmi4_data, work_dvfs_chg.work);
	int retval = 0;

	mutex_lock(&rmi4_data->dvfs_lock);

	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_DUAL) {
		if (rmi4_data->stay_awake) {
			dev_info(&rmi4_data->i2c_client->dev,
					"%s: do fw update, do not change cpu frequency.\n",
					__func__);
		} else {
			retval = set_freq_limit(DVFS_TOUCH_ID,
					MIN_TOUCH_LIMIT_SECOND);
			rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		}
	} else if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NINTH) {
		if (rmi4_data->stay_awake) {
			dev_info(&rmi4_data->i2c_client->dev,
					"%s: do fw update, do not change cpu frequency.\n",
					__func__);
		} else {
			retval = set_freq_limit(DVFS_TOUCH_ID,
					MIN_TOUCH_LIMIT);
			rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT;
		}
	} else if ((rmi4_data->dvfs_boost_mode == DVFS_STAGE_SINGLE) ||
			(rmi4_data->dvfs_boost_mode == DVFS_STAGE_TRIPLE) ||
			(rmi4_data->dvfs_boost_mode == DVFS_STAGE_PENTA)) {
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		rmi4_data->dvfs_freq = -1;
	}

	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: booster change failed(%d).\n",
				__func__, retval);
	mutex_unlock(&rmi4_data->dvfs_lock);
}

static void synaptics_set_dvfs_off(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
				struct synaptics_rmi4_data, work_dvfs_off.work);
	int retval;

	if (rmi4_data->stay_awake) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: do fw update, do not change cpu frequency.\n",
				__func__);
	} else {
		mutex_lock(&rmi4_data->dvfs_lock);

		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		rmi4_data->dvfs_freq = -1;

		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
		rmi4_data->dvfs_lock_status = false;

		mutex_unlock(&rmi4_data->dvfs_lock);
	}
}

static void synaptics_set_dvfs_lock(struct synaptics_rmi4_data *rmi4_data,
		int on)
{
	int ret = 0;

	if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NONE) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: DVFS stage is none(%d)\n",
				__func__, rmi4_data->dvfs_boost_mode);
		return;
	}

	mutex_lock(&rmi4_data->dvfs_lock);
	if (on == 0) {
		if (rmi4_data->dvfs_lock_status) {
			if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NINTH)
				schedule_delayed_work(&rmi4_data->work_dvfs_off,
					msecs_to_jiffies(TOUCH_BOOSTER_HIGH_OFF_TIME));
			else
				schedule_delayed_work(&rmi4_data->work_dvfs_off,
					msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));
		}
	} else if (on > 0) {
		cancel_delayed_work(&rmi4_data->work_dvfs_off);

		if ((!rmi4_data->dvfs_lock_status) || (rmi4_data->dvfs_old_stauts < on)) {
			cancel_delayed_work(&rmi4_data->work_dvfs_chg);

			if ((rmi4_data->dvfs_freq != MIN_TOUCH_LIMIT) &&
					(rmi4_data->dvfs_boost_mode != DVFS_STAGE_NINTH)) {
				if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_TRIPLE)
					ret = set_freq_limit(DVFS_TOUCH_ID,
						MIN_TOUCH_LIMIT_SECOND);
				else if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_PENTA)
					ret = set_freq_limit(DVFS_TOUCH_ID,
						MIN_TOUCH_LOW_LIMIT);
				else
					ret = set_freq_limit(DVFS_TOUCH_ID,
						MIN_TOUCH_LIMIT);
				rmi4_data->dvfs_freq = MIN_TOUCH_LIMIT;

				if (ret < 0)
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: cpu first lock failed(%d)\n",
							__func__, ret);
			} else if ((rmi4_data->dvfs_freq != MIN_TOUCH_HIGH_LIMIT) &&
					(rmi4_data->dvfs_boost_mode == DVFS_STAGE_NINTH)) {
				ret = set_freq_limit(DVFS_TOUCH_ID,
							MIN_TOUCH_HIGH_LIMIT);
				rmi4_data->dvfs_freq = MIN_TOUCH_HIGH_LIMIT;

				if (ret < 0)
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: cpu first lock failed(%d)\n",
							__func__, ret);
			}

			if (rmi4_data->dvfs_boost_mode == DVFS_STAGE_NINTH)
				schedule_delayed_work(&rmi4_data->work_dvfs_chg,
					msecs_to_jiffies(TOUCH_BOOSTER_HIGH_CHG_TIME));
			else
			schedule_delayed_work(&rmi4_data->work_dvfs_chg,
					msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

			rmi4_data->dvfs_lock_status = true;
		}
	} else if (on < 0) {
		if (rmi4_data->dvfs_lock_status) {
			cancel_delayed_work(&rmi4_data->work_dvfs_off);
			cancel_delayed_work(&rmi4_data->work_dvfs_chg);
			schedule_work(&rmi4_data->work_dvfs_off.work);
		}
	}
	rmi4_data->dvfs_old_stauts = on;
	mutex_unlock(&rmi4_data->dvfs_lock);
}

static void synaptics_init_dvfs(struct synaptics_rmi4_data *rmi4_data)
{
	mutex_init(&rmi4_data->dvfs_lock);

	rmi4_data->dvfs_boost_mode = DVFS_STAGE_DUAL;

	INIT_DELAYED_WORK(&rmi4_data->work_dvfs_off, synaptics_set_dvfs_off);
	INIT_DELAYED_WORK(&rmi4_data->work_dvfs_chg, synaptics_change_dvfs_lock);

	rmi4_data->dvfs_lock_status = false;
}
#endif

#ifdef TKEY_BOOSTER
static void synaptics_tkey_change_dvfs_lock(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
			struct synaptics_rmi4_data, work_tkey_dvfs_chg.work);
	int retval = 0;
	mutex_lock(&rmi4_data->tkey_dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, rmi4_data->tkey_dvfs_freq);
	if (retval < 0)
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: booster change failed(%d).\n",
			__func__, retval);
	rmi4_data->tkey_dvfs_lock_status = false;
	mutex_unlock(&rmi4_data->tkey_dvfs_lock);
}

static void synaptics_tkey_set_dvfs_off(struct work_struct *work)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(work,
			struct synaptics_rmi4_data, work_tkey_dvfs_off.work);
	int retval;

	if (rmi4_data->stay_awake) {
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: do fw update, do not change cpu frequency.\n",
				__func__);
	} else {
		mutex_lock(&rmi4_data->tkey_dvfs_lock);
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0)
			dev_info(&rmi4_data->i2c_client->dev,
				"%s: booster stop failed(%d).\n",
				__func__, retval);

		rmi4_data->tkey_dvfs_lock_status = true;
		mutex_unlock(&rmi4_data->tkey_dvfs_lock);
	}
}

static void synaptics_tkey_set_dvfs_lock(struct synaptics_rmi4_data *rmi4_data,
		int on)
{
	int ret = 0;

	if (rmi4_data->tkey_dvfs_boost_mode == DVFS_STAGE_NONE) {
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: DVFS stage is none(%d)\n",
				__func__, rmi4_data->tkey_dvfs_boost_mode);
		return;
	}

	mutex_lock(&rmi4_data->tkey_dvfs_lock);
	if (on == 0) {
		cancel_delayed_work(&rmi4_data->work_tkey_dvfs_chg);

		if (rmi4_data->tkey_dvfs_lock_status) {
			ret = set_freq_limit(DVFS_TOUCH_ID, rmi4_data->tkey_dvfs_freq);
			if (ret < 0)
				dev_info(&rmi4_data->i2c_client->dev,
					"%s: cpu first lock failed(%d)\n", __func__, ret);
			rmi4_data->tkey_dvfs_lock_status = false;
		}

		schedule_delayed_work(&rmi4_data->work_tkey_dvfs_off,
			msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));

	} else if (on == 1) {
		cancel_delayed_work(&rmi4_data->work_tkey_dvfs_off);
		schedule_delayed_work(&rmi4_data->work_tkey_dvfs_chg,
			msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));

	} else if (on == 2) {
		if (rmi4_data->tkey_dvfs_lock_status) {
			cancel_delayed_work(&rmi4_data->work_tkey_dvfs_off);
			cancel_delayed_work(&rmi4_data->work_tkey_dvfs_chg);
			schedule_work(&rmi4_data->work_tkey_dvfs_off.work);
		}
	}
	mutex_unlock(&rmi4_data->tkey_dvfs_lock);
}


static void synaptics_tkey_init_dvfs(struct synaptics_rmi4_data *rmi4_data)
{
	mutex_init(&rmi4_data->tkey_dvfs_lock);
	rmi4_data->tkey_dvfs_boost_mode = DVFS_STAGE_DUAL;
	rmi4_data->tkey_dvfs_freq = MIN_TOUCH_LIMIT_SECOND;

	INIT_DELAYED_WORK(&rmi4_data->work_tkey_dvfs_off, synaptics_tkey_set_dvfs_off);
	INIT_DELAYED_WORK(&rmi4_data->work_tkey_dvfs_chg, synaptics_tkey_change_dvfs_lock);

	rmi4_data->tkey_dvfs_lock_status = true;
}
#endif

static ssize_t synaptics_rmi4_f01_reset_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int reset;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &reset) != 1)
		return -EINVAL;

	if (reset != 1)
		return -EINVAL;

	retval = synaptics_rmi4_reset_device(rmi4_data);
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to issue reset command, error = %d\n",
				__func__, retval);
		return retval;
	}

	return count;
}

static ssize_t synaptics_rmi4_f01_productinfo_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "0x%02x 0x%02x\n",
			(rmi4_data->rmi4_mod_info.product_info[0]),
			(rmi4_data->rmi4_mod_info.product_info[1]));
}

static ssize_t synaptics_rmi4_f01_buildid_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned int build_id;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	build_id = (unsigned int)rmi->build_id[0] +
		(unsigned int)rmi->build_id[1] * 0x100 +
		(unsigned int)rmi->build_id[2] * 0x10000;

	return snprintf(buf, PAGE_SIZE, "%u\n",
			build_id);
}

static ssize_t synaptics_rmi4_f01_flashprog_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int retval;
	struct synaptics_rmi4_f01_device_status device_status;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			device_status.data,
			sizeof(device_status.data));
	if (retval < 0) {
		dev_err(dev,
				"%s: Failed to read device status, error = %d\n",
				__func__, retval);
		return retval;
	}

	return snprintf(buf, PAGE_SIZE, "%u\n",
			device_status.flash_prog);
}

static ssize_t synaptics_rmi4_0dbutton_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%u\n",
			rmi4_data->button_0d_enabled);
}

static ssize_t synaptics_rmi4_0dbutton_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;
	unsigned char ii;
	unsigned char intr_enable;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	input = input > 0 ? 1 : 0;

	if (rmi4_data->button_0d_enabled == input)
		return count;

	if (list_empty(&rmi->support_fn_list))
		return -ENODEV;

	list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
		if (fhandler->fn_number == SYNAPTICS_RMI4_F1A) {
			ii = fhandler->intr_reg_num;

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					rmi4_data->f01_ctrl_base_addr + 1 + ii,
					&intr_enable,
					sizeof(intr_enable));
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
				return retval;
			}

			if (input == 1)
				intr_enable |= fhandler->intr_mask;
			else
				intr_enable &= ~fhandler->intr_mask;

			retval = synaptics_rmi4_i2c_write(rmi4_data,
					rmi4_data->f01_ctrl_base_addr + 1 + ii,
					&intr_enable,
					sizeof(intr_enable));
			if (retval < 0)
				return retval;
		}
	}

	rmi4_data->button_0d_enabled = input;

	return count;
}

static ssize_t synaptics_rmi4_suspend_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1)
		return -EINVAL;

	if (input == 1)
		synaptics_rmi4_suspend(dev);
	else if (input == 0)
		synaptics_rmi4_resume(dev);
	else
		return -EINVAL;

	return count;
}

/**
 * synaptics_rmi4_set_page()
 *
 * Called by synaptics_rmi4_i2c_read() and synaptics_rmi4_i2c_write().
 *
 * This function writes to the page select register to switch to the
 * assigned page.
 */
static int synaptics_rmi4_set_page(struct synaptics_rmi4_data *rmi4_data,
		unsigned int address)
{
	int retval = 0;
	unsigned char retry;
	unsigned char buf[PAGE_SELECT_LEN];
	unsigned char page;
	struct i2c_client *i2c = rmi4_data->i2c_client;

	page = ((address >> 8) & MASK_8BIT);
	if (page != rmi4_data->current_page) {
		buf[0] = MASK_8BIT;
		buf[1] = page;
		for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
			retval = i2c_master_send(i2c, buf, PAGE_SELECT_LEN);
			if (retval != PAGE_SELECT_LEN) {
				if ((rmi4_data->tsp_probe != true) && (retry>=1)) {
					dev_err(&i2c->dev,
							"%s: TSP needs to reboot\n", __func__);
					retval = TSP_NEEDTO_REBOOT;
					return retval;
				}
				dev_err(&i2c->dev,
						"%s: I2C retry = %d, i2c_master_send retval = %d\n",
						__func__, retry + 1, retval);
				if (retval == 0)
					retval = -EAGAIN;
				msleep(20);
			} else {
				rmi4_data->current_page = page;
				break;
			}
		}
	} else {
		retval = PAGE_SELECT_LEN;
	}

	return retval;
}

/**
 * synaptics_rmi4_i2c_read()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function reads data of an arbitrary length from the sensor,
 * starting from an assigned register address of the sensor, via I2C
 * with a retry mechanism.
 */
static int synaptics_rmi4_i2c_read(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf;
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = 1,
			.buf = &buf,
		},
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		},
	};

	buf = addr & MASK_8BIT;

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Sensor stopped\n",
				__func__);
		retval = 0;
		goto exit;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 2) == 2) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev, "%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C read over retry limit\n",
				__func__);
		retval = -EIO;
	}
exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

/**
 * synaptics_rmi4_i2c_write()
 *
 * Called by various functions in this driver, and also exported to
 * other expansion Function modules such as rmi_dev.
 *
 * This function writes data of an arbitrary length to the sensor,
 * starting from an assigned register address of the sensor, via I2C with
 * a retry mechanism.
 */
static int synaptics_rmi4_i2c_write(struct synaptics_rmi4_data *rmi4_data,
		unsigned short addr, unsigned char *data, unsigned short length)
{
	int retval;
	unsigned char retry;
	unsigned char buf[length + 1];
	struct i2c_msg msg[] = {
		{
			.addr = rmi4_data->i2c_client->addr,
			.flags = 0,
			.len = length + 1,
			.buf = buf,
		}
	};

	mutex_lock(&(rmi4_data->rmi4_io_ctrl_mutex));

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: Sensor stopped\n",
				__func__);
		retval = 0;
		goto exit;
	}

	retval = synaptics_rmi4_set_page(rmi4_data, addr);
	if (retval != PAGE_SELECT_LEN)
		goto exit;

	buf[0] = addr & MASK_8BIT;
	memcpy(&buf[1], &data[0], length);

	for (retry = 0; retry < SYN_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(rmi4_data->i2c_client->adapter, msg, 1) == 1) {
			retval = length;
			break;
		}
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C retry %d\n",
				__func__, retry + 1);
		msleep(20);
	}

	if (retry == SYN_I2C_RETRY_TIMES) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: I2C write over retry limit\n",
				__func__);
		retval = -EIO;
	}
exit:
	mutex_unlock(&(rmi4_data->rmi4_io_ctrl_mutex));

	return retval;
}

/**
 * synaptics_rmi4_f11_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $11
 * finger data has been detected.
 *
 * This function reads the Function $11 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f11_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0; /* number of touch points */
	unsigned char reg_index;
	unsigned char finger;
	unsigned char fingers_supported;
	unsigned char num_of_finger_status_regs;
	unsigned char finger_shift;
	unsigned char finger_status;
	unsigned char data_reg_blk_size;
	unsigned char finger_status_reg[3];
	unsigned char data[F11_STD_DATA_LEN];
	unsigned short data_addr;
	unsigned short data_offset;
	int x;
	int y;
	int wx;
	int wy;
	int temp;

	/*
	 * The number of finger status registers is determined by the
	 * maximum number of fingers supported - 2 bits per finger. So
	 * the number of finger status registers to read is:
	 * register_count = ceil(max_num_of_fingers / 4)
	 */
	fingers_supported = fhandler->num_of_data_points;
	num_of_finger_status_regs = (fingers_supported + 3) / 4;
	data_addr = fhandler->full_addr.data_base;
	data_reg_blk_size = fhandler->size_of_data_register_block;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr,
			finger_status_reg,
			num_of_finger_status_regs);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
		return 0;
	}

	for (finger = 0; finger < fingers_supported; finger++) {
		reg_index = finger / 4;
		finger_shift = (finger % 4) * 2;
		finger_status = (finger_status_reg[reg_index] >> finger_shift)
			& MASK_2BIT;

		/*
		 * Each 2-bit finger status field represents the following:
		 * 00 = finger not present
		 * 01 = finger present and data accurate
		 * 10 = finger present but data may be inaccurate
		 * 11 = reserved
		 */
		input_mt_slot(rmi4_data->input_dev, finger);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, finger_status);

		if (finger_status) {
			data_offset = data_addr +
				num_of_finger_status_regs +
				(finger * data_reg_blk_size);
			retval = synaptics_rmi4_i2c_read(rmi4_data,
					data_offset,
					data,
					data_reg_blk_size);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
				return 0;
			}

			x = (data[0] << 4) | (data[2] & MASK_4BIT);
			y = (data[1] << 4) | ((data[2] >> 4) & MASK_4BIT);
			wx = (data[3] & MASK_4BIT);
			wy = (data[3] >> 4) & MASK_4BIT;

			if (rmi4_data->dt_data->swap_axes) {
				temp = x;
				x = y;
				y = temp;
				temp = wx;
				wx = wy;
				wy = temp;
			}

			if (rmi4_data->dt_data->x_flip)
				x = rmi4_data->sensor_max_x - x;
			if (rmi4_data->dt_data->y_flip)
				y = rmi4_data->sensor_max_y - y;

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif

			dev_dbg(&rmi4_data->i2c_client->dev,
					"%s: Finger %d:\n"
					"status = 0x%02x\n"
					"x = %d\n"
					"y = %d\n"
					"wx = %d\n"
					"wy = %d\n",
					__func__, finger,
					finger_status,
					x, y, wx, wy);

			touch_count++;
		}
	}
/*
	if (touch_count == 0) {
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOOL_FINGER, 0);
	}
*/
	input_sync(rmi4_data->input_dev);

	return touch_count;
}

/**
 * synaptics_rmi4_f12_abs_report()
 *
 * Called by synaptics_rmi4_report_touch() when valid Function $12
 * finger data has been detected.
 *
 * This function reads the Function $12 data registers, determines the
 * status of each finger supported by the Function, processes any
 * necessary coordinate manipulation, reports the finger data to
 * the input subsystem, and returns the number of fingers detected.
 */
static int synaptics_rmi4_f12_abs_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0;
	unsigned char finger;
	unsigned char fingers_to_process;
	unsigned char finger_status;
	unsigned char size_of_2d_data;
	unsigned short data_addr;
	int x;
	int y;
	int wx = 0;
	int wy = 0;
	struct synaptics_rmi4_f12_extra_data *extra_data;
	struct synaptics_rmi4_f12_finger_data *data;
	struct synaptics_rmi4_f12_finger_data *finger_data;
#ifdef EDGE_SWIPE_SCALE
	int scailing_width;
#endif
#ifdef REDUCE_I2C_DATA_LENGTH
	static unsigned char fingers_already_present;
	unsigned short d_len;
#endif
	unsigned char tool_type = MT_TOOL_FINGER;

	fingers_to_process = fhandler->num_of_data_points;
	data_addr = fhandler->full_addr.data_base;
	extra_data = (struct synaptics_rmi4_f12_extra_data *)fhandler->extra;
	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);
#ifdef EDGE_SWIPE_SCALE
	scailing_width = (rmi4_data->f51_handle->surface_data.sumsize) * 8 / 5;
#endif

#ifdef REDUCE_I2C_DATA_LENGTH
	if (extra_data->data15_size) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				data_addr + extra_data->data15_offset,
				extra_data->data15_data,
				extra_data->data15_size);
		if (retval < 0)
			return 0;

		d_len = (extra_data->data15_data[0] | ((extra_data->data15_data[1] & 0x3) << 8));

		/* d_len : [00000011 11111111] : "1" is valid finger data. check Highest Finger ID */
		for (fingers_to_process = 10; fingers_to_process > 0; fingers_to_process--) {
			if (d_len & (1 << (fingers_to_process - 1)))
				break;
		}
	}

	fingers_to_process = max(fingers_to_process, fingers_already_present);
#endif

	if (!fingers_to_process) {
		synaptics_rmi4_free_fingers(rmi4_data);
		return 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr + extra_data->data1_offset,
			(unsigned char *)fhandler->data,
			fingers_to_process * size_of_2d_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return 0;
	}
	data = (struct synaptics_rmi4_f12_finger_data *)fhandler->data;

	for (finger = 0; finger < fingers_to_process; finger++) {
		finger_data = data + finger;
		finger_status = (finger_data->object_type_and_status & MASK_3BIT);
		x = (finger_data->x_msb << 8) | (finger_data->x_lsb);
		y = (finger_data->y_msb << 8) | (finger_data->y_lsb);
		if ((x == INVALID_X) && (y == INVALID_Y))
			finger_status = 0;

		/*
		  * Each 3-bit finger status field represents the following:
		  * 000 = finger not present
		  * 001 = finger present and data accurate
		  * 010 = stylus pen (passive pen)
		  * 011 = palm touch
		  * 100 = not used
		  * 101 = hover
		  * 110 = glove touch
		 */
#ifdef USE_STYLUS
		tool_type = MT_TOOL_FINGER;
		if ((finger_status == STYLUS_PRESSED) && rmi4_data->use_stylus)
			tool_type = MT_TOOL_PEN;
#endif
		input_mt_slot(rmi4_data->input_dev, finger);
			input_mt_report_slot_state(rmi4_data->input_dev,
				tool_type, finger_status ? true : false);

		if (finger_status) {
#ifdef REDUCE_I2C_DATA_LENGTH
			fingers_already_present = finger + 1;
#endif
#ifdef GLOVE_MODE
			if (((finger_status == GLOVE_PRESSED) || (finger_status == STYLUS_PRESSED)) &&
				!rmi4_data->touchkey_glove_mode_status) {
				rmi4_data->touchkey_glove_mode_status = true;
				input_report_switch(rmi4_data->input_dev,
						SW_GLOVE, true);
			} else if (((finger_status != GLOVE_PRESSED) && (finger_status != STYLUS_PRESSED)) &&
				rmi4_data->touchkey_glove_mode_status) {
				rmi4_data->touchkey_glove_mode_status = false;
				input_report_switch(rmi4_data->input_dev,
						SW_GLOVE, false);
			}
#endif

#ifdef REPORT_2D_W
			wx = finger_data->wx;
			wy = finger_data->wy;

#ifdef EDGE_SWIPE
			if (rmi4_data->f51_handle) {
#ifdef USE_PALM_REJECTION_KERNEL
				if ((finger_status == PALM_PRESSED) &&
					(rmi4_data->f51_handle->surface_data.palm == 0)) {
					dev_info(&rmi4_data->i2c_client->dev,
							"%s: finger status 0x3, but No Palm. Reject!\n",
							__func__);

					continue;
				}
#endif
#if !defined(CONFIG_SEC_FACTORY)
				if (rmi4_data->f51_handle->general_control & HAS_EDGE_SWIPE) {
#ifdef USE_PALM_REJECTION_KERNEL
					if (rmi4_data->f51_handle->surface_data.palm) {
#else
					if (finger_status == PALM_PRESSED) {
#endif
						wx = rmi4_data->f51_handle->surface_data.wx;
						wy = rmi4_data->f51_handle->surface_data.wy;
					}
#ifdef USE_EDGE_SWIPE_WIDTH_MAJOR
#ifdef EDGE_SWIPE_SCALE
					input_report_abs(rmi4_data->input_dev,
							ABS_MT_WIDTH_MAJOR, scailing_width);
#else
					input_report_abs(rmi4_data->input_dev,
							ABS_MT_WIDTH_MAJOR, rmi4_data->f51_handle->surface_data.sumsize);
#endif
#endif

					input_report_abs(rmi4_data->input_dev,
							ABS_MT_PALM, rmi4_data->f51_handle->surface_data.palm);
				}
#endif	/* CONFIG_SEC_FACTORY */
			}
#endif	/* EDGE_SWIPE */
#endif	/* REPORT_2D_W */


			if (rmi4_data->dt_data->tablet > 0) {
				x = rmi4_data->sensor_max_x - x;
				y = rmi4_data->sensor_max_y - y;
			}

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 1);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
#ifdef REPORT_2D_W
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MAJOR, max(wx, wy));
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_TOUCH_MINOR, min(wx, wy));
#endif

			if (!rmi4_data->finger[finger].state) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&rmi4_data->i2c_client->dev,
						"[%d][P] 0x%02x, x = %d, y = %d, wx = %d, wy = %d |[%d]\n",
						finger, finger_status, x, y, wx, wy, fingers_to_process);
#else
				dev_info(&rmi4_data->i2c_client->dev,
						"[%d][P] 0x%02x\n",
						finger, finger_status);
#endif
			} else {
				rmi4_data->finger[finger].mcount++;
			}

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			if (rmi4_data->debug_log & 0x2)
				dev_info(&rmi4_data->i2c_client->dev,
						"[%d] 0x%02x, x = %d, y = %d, wx = %d, wy = %d |[%d]\n",
						finger, finger_status, x, y, wx, wy, fingers_to_process);
#endif
			touch_count++;
		}

		if (rmi4_data->finger[finger].state && (!finger_status)) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
			dev_info(&rmi4_data->i2c_client->dev, "[%d][R] 0x%02x M[%d], Ver[%02X%02X][%X/%d]\n",
					finger, finger_status, rmi4_data->finger[finger].mcount,
					rmi4_data->ic_revision_of_ic, rmi4_data->fw_version_of_ic, rmi4_data->lcd_id, system_rev);
#else
			dev_info(&rmi4_data->i2c_client->dev, "[%d][R] 0x%02x M[%d], Ver[%02X%02X][%X/%d]\n",
					finger, finger_status, rmi4_data->finger[finger].mcount,
					rmi4_data->ic_revision_of_ic, rmi4_data->fw_version_of_ic, rmi4_data->lcd_id, system_rev);
#endif
			rmi4_data->finger[finger].mcount = 0;
		}

		rmi4_data->finger[finger].state = finger_status;

	}

		/* Clear BTN_TOUCH when All touch are released  */
	if (touch_count == 0) {
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
#ifdef REDUCE_I2C_DATA_LENGTH
		fingers_already_present = 0;
#endif
	}

	input_sync(rmi4_data->input_dev);

#ifdef COMMON_INPUT_BOOSTER
	if (rmi4_data->tsp_booster->dvfs_set)
		rmi4_data->tsp_booster->dvfs_set(rmi4_data->tsp_booster, touch_count);
#endif

#ifdef TSP_BOOSTER
	if (touch_count)
		synaptics_set_dvfs_lock(rmi4_data, touch_count);
	else
		synaptics_set_dvfs_lock(rmi4_data, 0);
#endif

	return touch_count;
}

static void synaptics_rmi4_f1a_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char touch_count = 0;
	unsigned char button;
	unsigned char index;
	unsigned char shift;
	unsigned char status;
	unsigned char *data;
	unsigned short data_addr = fhandler->full_addr.data_base;
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;
	static unsigned char do_once = 1;
	static bool current_status[MAX_NUMBER_OF_BUTTONS];
#ifdef NO_0D_WHILE_2D
	static bool before_2d_status[MAX_NUMBER_OF_BUTTONS];
	static bool while_2d_status[MAX_NUMBER_OF_BUTTONS];
#endif

	if (do_once) {
		memset(current_status, 0, sizeof(current_status));
#ifdef NO_0D_WHILE_2D
		memset(before_2d_status, 0, sizeof(before_2d_status));
		memset(while_2d_status, 0, sizeof(while_2d_status));
#endif
		do_once = 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			data_addr,
			f1a->button_data_buffer,
			f1a->button_bitmask_size);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read button data registers\n",
				__func__);
		return;
	}

	data = f1a->button_data_buffer;

	for (button = 0; button < f1a->valid_button_count; button++) {
		index = button / 8;
		shift = button % 8;
		status = ((data[index] >> shift) & MASK_1BIT);

		if (current_status[button] == status)
			continue;
		else
			current_status[button] = status;

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: Button %d (code %d) ->%d\n",
				__func__, button,
				f1a->button_map[button],
				status);
#ifdef NO_0D_WHILE_2D
		if (rmi4_data->fingers_on_2d == false) {
			if (status == 1) {
				before_2d_status[button] = 1;
			} else {
				if (while_2d_status[button] == 1) {
					while_2d_status[button] = 0;
					continue;
				} else {
					before_2d_status[button] = 0;
				}
			}
			touch_count++;
			input_report_key(rmi4_data->input_dev,
					f1a->button_map[button],
					status);
		} else {
			if (before_2d_status[button] == 1) {
				before_2d_status[button] = 0;
				touch_count++;
				input_report_key(rmi4_data->input_dev,
						f1a->button_map[button],
						status);
			} else {
				if (status == 1)
					while_2d_status[button] = 1;
				else
					while_2d_status[button] = 0;
			}
		}
#else
		touch_count++;
		input_report_key(rmi4_data->input_dev,
				f1a->button_map[button],
				status);
#endif
	}

#ifdef TKEY_BOOSTER
	synaptics_tkey_set_dvfs_lock(rmi4_data, !!status);
#endif
	if (touch_count)
		input_sync(rmi4_data->input_dev);

	return;
}

#ifdef PROXIMITY
#ifdef EDGE_SWIPE
static int synaptics_rmi4_f51_edge_swipe(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned short data_base_addr;
	struct synaptics_rmi4_f51_data *data;

	data_base_addr = fhandler->full_addr.data_base;
	data = (struct synaptics_rmi4_f51_data *)fhandler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->edge_swipe_data_addr,
			data->edge_swipe_data,
			sizeof(data->edge_swipe_data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
					__func__, retval);
		return retval;
	}

	if (!rmi4_data->f51_handle)
		return -ENODEV;
	rmi4_data->f51_handle->surface_data.sumsize = data->edge_swipe_mm;
	rmi4_data->f51_handle->surface_data.wx = data->edge_swipe_wx;
	rmi4_data->f51_handle->surface_data.wy = data->edge_swipe_wy;
	rmi4_data->f51_handle->surface_data.palm = data->edge_swipe_z;

#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	if (rmi4_data->debug_log & 0x1)
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: edge_data : x[%d], y[%d], z[%d] ,wx[%d], wy[%d], area[%d], angle[%d][%d]\n", __func__,
				data->edge_swipe_x_msb << 8 | data->edge_swipe_x_lsb,
				data->edge_swipe_y_msb << 8 | data->edge_swipe_y_lsb,
				data->edge_swipe_z, data->edge_swipe_wx, data->edge_swipe_wy,
				data->edge_swipe_mm, data->edge_swipe_dg, rmi4_data->f51_handle->surface_data.angle);
#endif
	return retval;
}
#endif

#ifdef SIDE_TOUCH
static int synaptics_rmi4_f51_side_btns(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;

	int status;
	unsigned char touch_count = 0;
	unsigned char button;

	unsigned short data_base_addr;
	struct synaptics_rmi4_f51_data *data;

	data_base_addr = fhandler->full_addr.data_base;
	data = (struct synaptics_rmi4_f51_data *)fhandler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->side_button_data_addr,
			data->side_button_data,
			sizeof(data->side_button_data));
	if (retval < 0)
		return retval;

	if (!data->side_button_data[0] && !data->side_button_data[1])
		return 0;

	for (button = 0; button < 8; button++) {
		if (data->side_button_leading & (1 << button))
			status = 1;
		else if (data->side_button_trailing & (1 << button))
			status = 0;
		else
			status = -1;

		if (status == 1)
			rmi4_data->sidekey_data |= data->side_button_leading;
		else if (status == 0)
			rmi4_data->sidekey_data &= ~(data->side_button_trailing);

		if (status >= 0) {
			touch_count++;
					input_report_key(rmi4_data->input_dev,
						KEY_SIDE_TOUCH_0 + button, status);

			dev_info(&rmi4_data->i2c_client->dev,
					"%s: SIDE_KEY #%d %s[%X], CAM[%X]\n",
					__func__, button, status ? "PRESS" : "RELEASE",
					rmi4_data->sidekey_data,
					data->side_button_cam_det);
		}
	}

	if (data->side_button_cam_det & MASK_1BIT)
		input_report_key(rmi4_data->input_dev, KEY_SIDE_CAMERA_DETECTED, 1);
	else
		input_report_key(rmi4_data->input_dev, KEY_SIDE_CAMERA_DETECTED, 0);

	if (touch_count)
		input_sync(rmi4_data->input_dev);
	else
		rmi4_data->sidekey_data = 0;

	return 0;
}

#endif

/*
 * Detection flag : Face | Profile Handedness | LOWG | Hover Pinch | Large Object | Air Swipe | Hover
 * Detection flag2 : Side button | Hand Edge Swipe
 */
static int synaptics_rmi4_f51_detection_flag(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	struct synaptics_rmi4_f51_data *data;
	int retval;

	data = (struct synaptics_rmi4_f51_data *)fhandler->data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.data_base,
			data->proximity_data,
			sizeof(data->proximity_data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read proximity data registers\n",
				__func__);
		return retval;
	}

#ifdef HAND_GRIP_MODE
	/* enable hover event + handgrip event */
	if (rmi4_data->hand_grip_mode && (data->proximity_data[0] & PROFILE_HANDEDNESS_DETECTED)) {
		dev_dbg(&rmi4_data->i2c_client->dev,
		"%s: profile_handedness_status, 0x%x\n",
		__func__, data->profile_handedness_status);

		if (rmi4_data->old_code != data->profile_handedness_status) {
			if (data->profile_handedness_status == 0x2)
				rmi4_data->hand_grip = SW_RIGHT_HAND;
			else if (data->profile_handedness_status == 0x1)
				rmi4_data->hand_grip = SW_LEFT_HAND;
			else
				rmi4_data->hand_grip = SW_BOTH_HAND;

			input_report_switch(rmi4_data->input_dev, rmi4_data->old_hand_grip, 0);
			input_sync(rmi4_data->input_dev);

			input_report_switch(rmi4_data->input_dev,
					rmi4_data->hand_grip, 1);

			input_sync(rmi4_data->input_dev);
			dev_info(&rmi4_data->i2c_client->dev, "%s: (%d) %s Hand grip.(old = %s)\n",
					__func__, data->profile_handedness_status,
					rmi4_data->hand_grip == SW_RIGHT_HAND ? "RIGHT" :
					rmi4_data->hand_grip == SW_LEFT_HAND ? "LEFT" : "BOTH",
					rmi4_data->old_hand_grip == SW_RIGHT_HAND ? "RIGHT" :
					rmi4_data->old_hand_grip == SW_LEFT_HAND ? "LEFT" : "BOTH");

		}

	rmi4_data->old_code = data->profile_handedness_status;
	rmi4_data->old_hand_grip = rmi4_data->hand_grip;

	if (rmi4_data->hand_grip_mode && !rmi4_data->hover_status_in_normal_mode)
		return 0;

	}
#endif

	if (!rmi4_data->dt_data->surface_only) {
		if (data->proximity_data[0] == HOVER_UN_DETECTED) {
			if (rmi4_data->f51_finger_is_hover) {
				dev_info (&rmi4_data->i2c_client->dev,
					"%s: Hover finger[OUT]\n", __func__);
				rmi4_data->f51_finger_is_hover = false;
			}
			return retval;
		}
		if ((data->finger_hover_det & HOVER_DETECTED) && (data->hover_finger_z > 0)) {
			int x, y, z;
			x = (data->hover_finger_x_4__11 << 4) |
				(data->hover_finger_xy_0__3 & 0x0f);
			y = (data->hover_finger_y_4__11 << 4) |
				(data->hover_finger_xy_0__3 >> 4);
			z = HOVER_Z_MAX - data->hover_finger_z;

			input_mt_slot(rmi4_data->input_dev, 0);
			input_mt_report_slot_state(rmi4_data->input_dev,
					MT_TOOL_FINGER, 1);

			input_report_key(rmi4_data->input_dev,
					BTN_TOUCH, 0);
			input_report_key(rmi4_data->input_dev,
					BTN_TOOL_FINGER, 1);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_X, x);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_POSITION_Y, y);
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_DISTANCE, z);

			input_sync(rmi4_data->input_dev);

			if (!rmi4_data->f51_finger_is_hover) {
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
				dev_info(&rmi4_data->i2c_client->dev,
					"Hover finger[IN]: x = %d, y = %d, z = %d\n", x, y, z);
#else
				dev_info(&rmi4_data->i2c_client->dev,
					"Hover finger[IN]\n");
#endif
				rmi4_data->f51_finger_is_hover = true;
			}

			rmi4_data->f51_finger = true;
			rmi4_data->fingers_on_2d = false;
			synaptics_rmi4_f51_finger_timer((unsigned long)rmi4_data);
		}
	}

	if (data->air_swipe_det || data->large_obj_det || data->hover_pinch_det || data->object_present)
		dev_dbg(&rmi4_data->i2c_client->dev,
				"%s: air_d_0: %d, air_d_1: %d, large: %d, pinch: %d, present: %d\n",
				__func__, data->air_swipe_dir_0, data->air_swipe_dir_1,
				data->large_obj_act, data->hover_pinch_dir, data->object_present);

	return retval;
}

static int synaptics_rmi4_f51_detection_flag2(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval = 0;
	unsigned char detection_flag2 = 0xFF;
	unsigned char wx_wy_sumsize[3] = {0, };

#ifdef USE_DETECTION_FLAG2
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->detection_flag2_addr,
			&detection_flag2, 1);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read detection flag2\n",
				__func__);
	}
#endif

#ifdef EDGE_SWIPE
		/* Read edge swipe data */
	if (detection_flag2 & EDGE_SWIPE_DETECTED) {
		if ((rmi4_data->f51_handle->general_control & EDGE_SWIPE_EN) &&
				rmi4_data->fingers_on_2d) {
			retval = synaptics_rmi4_f51_edge_swipe(rmi4_data, fhandler);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read edge swipe data\n",
					__func__);
			return retval;
		}
	}
	} else {
		if (rmi4_data->f51_handle->surface_data.palm)
				rmi4_data->f51_handle->surface_data.palm = 0;

		retval = synaptics_rmi4_i2c_read(rmi4_data,
					rmi4_data->f51_handle->edge_swipe_data_addr + EDGE_SWIPE_SUMSIZE_OFFSET,
					wx_wy_sumsize, sizeof(wx_wy_sumsize));
		if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s: Failed to read sumsize data[%d]\n",
							__func__, retval);
			return retval;
		}

		rmi4_data->f51_handle->surface_data.sumsize = wx_wy_sumsize[2];
		rmi4_data->f51_handle->surface_data.wx = wx_wy_sumsize[0];
		rmi4_data->f51_handle->surface_data.wy = wx_wy_sumsize[1];
	}
#endif

#ifdef SIDE_TOUCH
	/* Read side touch / side button data */
	if ((detection_flag2 & SIDE_BUTTON_DETECTED) && rmi4_data->sidekey_enables) {
		retval = synaptics_rmi4_f51_side_btns(rmi4_data, fhandler);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read side button data\n",
					__func__);
			return retval;
		}
	}
#endif
	return retval;
	}

static void synaptics_rmi4_f51_report(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;

	retval = synaptics_rmi4_f51_detection_flag(rmi4_data, fhandler);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read detection_flag\n",
				__func__);
		return;
	}


	retval = synaptics_rmi4_f51_detection_flag2(rmi4_data, fhandler);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read detection_flag2\n",
				__func__);
		return;
	}

	return;
}
#endif

/**
 * synaptics_rmi4_report_touch()
 *
 * Called by synaptics_rmi4_sensor_report().
 *
 * This function calls the appropriate finger data reporting function
 * based on the function handler it receives and returns the number of
 * fingers detected.
 */
static void synaptics_rmi4_report_touch(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	unsigned char touch_count_2d;

	dev_dbg(&rmi4_data->i2c_client->dev,
			"%s: Function %02x reporting\n",
			__func__, fhandler->fn_number);

	switch (fhandler->fn_number) {
	case SYNAPTICS_RMI4_F11:
		touch_count_2d = synaptics_rmi4_f11_abs_report(rmi4_data,
				fhandler);
		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F12:
		touch_count_2d = synaptics_rmi4_f12_abs_report(rmi4_data,
				fhandler);
		if (touch_count_2d)
			rmi4_data->fingers_on_2d = true;
		else
			rmi4_data->fingers_on_2d = false;
		break;
	case SYNAPTICS_RMI4_F1A:
		synaptics_rmi4_f1a_report(rmi4_data, fhandler);
		break;
#ifdef PROXIMITY
	case SYNAPTICS_RMI4_F51:
		synaptics_rmi4_f51_report(rmi4_data, fhandler);
		break;
#endif
	default:
		dev_info(&rmi4_data->i2c_client->dev,
				"%s: fn_number : %d\n",
				__func__, fhandler->fn_number);
		break;
	}

	return;
}

/**
 * synaptics_rmi4_sensor_report()
 *
 * Called by synaptics_rmi4_irq().
 *
 * This function determines the interrupt source(s) from the sensor
 * and calls synaptics_rmi4_report_touch() with the appropriate
 * function handler for each function with valid data inputs.
 */
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data[MAX_INTR_REGISTERS + 1];
	unsigned char *intr = &data[1];
	struct synaptics_rmi4_f01_device_status status;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_exp_fn *exp_fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	/*
	 * Get interrupt status information from F01 Data1 register to
	 * determine the source(s) that are flagging the interrupt.
	 */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			data,
			rmi4_data->num_of_intr_regs + 1);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read interrupt status\n",
				__func__);
		return retval;
	}

	status.data[0] = data[0];
	if (status.unconfigured) {
		if (rmi4_data->doing_reflash) {
			dev_err(&rmi4_data->i2c_client->dev,
					"Spontaneous reset detected during reflash.\n");
			return 0;
		}

		dev_info(&rmi4_data->i2c_client->dev,
				"Spontaneous reset detected\n");
		retval = synaptics_rmi4_reinit_device(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to reinit device\n",
					__func__);
		}
		return 0;
	}

	/*
	 * Traverse the function handler list and service the source(s)
	 * of the interrupt accordingly.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				if (fhandler->intr_mask &
						intr[fhandler->intr_reg_num]) {
					synaptics_rmi4_report_touch(rmi4_data,
							fhandler);
				}
			}
		}
	}

	if (!list_empty(&exp_fn_list)) {
		list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
			if (exp_fhandler->initialized &&
					(exp_fhandler->func_attn != NULL))
				exp_fhandler->func_attn(rmi4_data, intr[0]);
		}
	}

	return 0;
}

/**
 * synaptics_rmi4_irq()
 *
 * Called by the kernel when an interrupt occurs (when the sensor
 * asserts the attention irq).
 *
 * This function is the ISR thread and handles the acquisition
 * and the reporting of finger data when the presence of fingers
 * is detected.
 */
static irqreturn_t synaptics_rmi4_irq(int irq, void *data)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = data;

	do {
		retval = synaptics_rmi4_sensor_report(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: Failed to read",
					__func__);
			goto out;
		}

		if (!rmi4_data->touch_stopped)
			goto out;

	} while (!gpio_get_value(rmi4_data->dt_data->irq_gpio));
out:
	return IRQ_HANDLED;
}

/**
 * synaptics_rmi4_irq_enable()
 *
 * Called by synaptics_rmi4_probe() and the power management functions
 * in this driver and also exported to other expansion Function modules
 * such as rmi_dev.
 *
 * This function handles the enabling and disabling of the attention
 * irq including the setting up of the ISR thread.
 */
int synaptics_rmi4_irq_enable(struct synaptics_rmi4_data *rmi4_data,
		bool enable)
{
	int retval = 0;
	unsigned char intr_status[MAX_INTR_REGISTERS];

	if (enable) {
		if (rmi4_data->irq_enabled)
			return retval;

		/* Clear interrupts first */
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr + 1,
				intr_status,
				rmi4_data->num_of_intr_regs);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
					__func__, retval);
			return retval;
		}

		if (rmi4_data->dt_data->extra_config[3])
		retval = request_threaded_irq(rmi4_data->irq, NULL,
				synaptics_rmi4_irq, TSP_IRQ_TYPE_LEVEL, 
				DRIVER_NAME, rmi4_data);
		else
			retval = request_threaded_irq(rmi4_data->irq, NULL,
				synaptics_rmi4_irq, TSP_IRQ_TYPE_EDGE, 
				DRIVER_NAME, rmi4_data);

		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to create irq thread\n",
					__func__);
			return retval;
		}

		rmi4_data->irq_enabled = true;
	} else {
		if (rmi4_data->irq_enabled) {
			disable_irq(rmi4_data->irq);
			free_irq(rmi4_data->irq, rmi4_data);
			rmi4_data->irq_enabled = false;
		}
	}

	return retval;
}

/**
 * synaptics_rmi4_f11_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 11 registers
 * and determines the number of fingers supported, x and y data ranges,
 * offset to the associated interrupt status register, interrupt bit
 * mask, and gathers finger data acquisition capabilities from the query
 * registers.
 */
static int synaptics_rmi4_f11_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char abs_data_size;
	unsigned char abs_data_blk_size;
	unsigned char query[F11_STD_QUERY_LEN];
	unsigned char control[F11_STD_CTRL_LEN];

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			query,
			sizeof(query));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%d read fail[%d]\n",
					__func__, __LINE__, retval);
		return retval;
	}

	/* Maximum number of fingers supported */
	if ((query[1] & MASK_3BIT) <= 4)
		fhandler->num_of_data_points = (query[1] & MASK_3BIT) + 1;
	else if ((query[1] & MASK_3BIT) == 5)
		fhandler->num_of_data_points = 10;

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base,
			control,
			sizeof(control));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%d read fail[%d]\n",
					__func__, __LINE__, retval);
		return retval;
	}

	/* Maximum x and y */
	rmi4_data->sensor_max_x = ((control[6] & MASK_8BIT) << 0) |
		((control[7] & MASK_4BIT) << 8);
	rmi4_data->sensor_max_y = ((control[8] & MASK_8BIT) << 0) |
		((control[9] & MASK_4BIT) << 8);
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: Function %02x max x = %d max y = %d\n",
			__func__, fhandler->fn_number,
			rmi4_data->sensor_max_x,
			rmi4_data->sensor_max_y);
	if (rmi4_data->sensor_max_x <= 0 || rmi4_data->sensor_max_y <= 0) {
		rmi4_data->sensor_max_x = rmi4_data->dt_data->coords[0];
		rmi4_data->sensor_max_y = rmi4_data->dt_data->coords[1];

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: Function %02x read failed, run dtsi coords. max x = %d max y = %d\n",
				__func__, fhandler->fn_number,
				rmi4_data->sensor_max_x,
				rmi4_data->sensor_max_y);
	}

	rmi4_data->max_touch_width = MAX_F11_TOUCH_WIDTH;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
				intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	abs_data_size = query[5] & MASK_2BIT;
	abs_data_blk_size = 3 + (2 * (abs_data_size == 0 ? 1 : 0));
	fhandler->size_of_data_register_block = abs_data_blk_size;
	fhandler->data = NULL;
	fhandler->extra = NULL;

	return retval;
}

int synaptics_rmi4_f12_ctrl11_set (struct synaptics_rmi4_data *rmi4_data,
		unsigned char data)
{
	struct synaptics_rmi4_f12_ctrl_11 ctrl_11;

	int retval;

	if (rmi4_data->touch_stopped)
		return 0;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f12_ctrl11_addr,
			ctrl_11.data,
			sizeof(ctrl_11.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
				__func__, retval);
		return retval;
	}

	ctrl_11.data[2] = data; /* set a value of jitter filter */

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f12_ctrl11_addr,
			ctrl_11.data,
			sizeof(ctrl_11.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: write fail[%d]\n",
				__func__, retval);
		return retval;
	}

	return 0;
}

int synaptics_rmi4_f12_set_feature(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (rmi4_data->has_glove_mode) {
		retval = synaptics_rmi4_i2c_write(rmi4_data,
				rmi4_data->f12_ctrl26_addr,
				&rmi4_data->feature_enable,
				sizeof(rmi4_data->feature_enable));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: write fail[%d]\n",
				__func__, retval);
			return retval;
		}

		dev_info(&rmi4_data->i2c_client->dev, "%s: 0x%x\n",
				__func__, rmi4_data->feature_enable);
	}

	return 0;
}

static int synaptics_rmi4_f12_set_report(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f12_ctrl28_addr,
			&rmi4_data->report_enable,
			sizeof(rmi4_data->report_enable));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: write fail[%d]\n",
				__func__, retval);
		return retval;
	}

	return retval;
}

/**
 * synaptics_rmi4_f12_init()
 *
 * Called by synaptics_rmi4_query_device().
 *
 * This funtion parses information from the Function 12 registers and
 * determines the number of fingers supported, offset to the data1
 * register, x and y data ranges, offset to the associated interrupt
 * status register, interrupt bit mask, and allocates memory resources
 * for finger data acquisition.
 */
static int synaptics_rmi4_f12_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned char intr_offset;
	unsigned char size_of_2d_data;
	unsigned char size_of_query8;
	unsigned char ctrl_8_offset;
	unsigned char ctrl_9_offset;
	unsigned char ctrl_11_offset;
	unsigned char ctrl_15_offset;
	unsigned char ctrl_23_offset;
	unsigned char ctrl_26_offset;
	unsigned char ctrl_28_offset;
	struct synaptics_rmi4_f12_extra_data *extra_data;
	struct synaptics_rmi4_f12_query_5 query_5;
	struct synaptics_rmi4_f12_query_8 query_8;
	struct synaptics_rmi4_f12_query_10 query_10;
	struct synaptics_rmi4_f12_ctrl_8 ctrl_8;
	struct synaptics_rmi4_f12_ctrl_9 ctrl_9;
	struct synaptics_rmi4_f12_ctrl_11 ctrl_11;
	struct synaptics_rmi4_f12_ctrl_23 ctrl_23;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;
	fhandler->extra = kzalloc(sizeof(*extra_data), GFP_KERNEL);
	extra_data = (struct synaptics_rmi4_f12_extra_data *)fhandler->extra;
	size_of_2d_data = sizeof(struct synaptics_rmi4_f12_finger_data);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 5,
			query_5.data,
			sizeof(query_5.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	ctrl_8_offset = query_5.ctrl0_is_present +
		query_5.ctrl1_is_present +
		query_5.ctrl2_is_present +
		query_5.ctrl3_is_present +
		query_5.ctrl4_is_present +
		query_5.ctrl5_is_present +
		query_5.ctrl6_is_present +
		query_5.ctrl7_is_present;

	ctrl_9_offset = ctrl_8_offset +
		query_5.ctrl8_is_present;

	ctrl_11_offset = ctrl_9_offset +
		query_5.ctrl9_is_present +
		query_5.ctrl10_is_present;

	ctrl_15_offset = ctrl_11_offset +
		query_5.ctrl11_is_present +
		query_5.ctrl12_is_present +
		query_5.ctrl13_is_present +
		query_5.ctrl14_is_present;

	ctrl_23_offset = ctrl_15_offset +
		query_5.ctrl15_is_present +
		query_5.ctrl16_is_present +
		query_5.ctrl17_is_present +
		query_5.ctrl18_is_present +
		query_5.ctrl19_is_present +
		query_5.ctrl20_is_present +
		query_5.ctrl21_is_present +
		query_5.ctrl22_is_present;

	ctrl_26_offset = ctrl_23_offset +
		query_5.ctrl23_is_present +
		query_5.ctrl24_is_present +
		query_5.ctrl25_is_present;

	ctrl_28_offset = ctrl_26_offset +
		query_5.ctrl26_is_present +
		query_5.ctrl27_is_present;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_23_offset,
			ctrl_23.data,
			sizeof(ctrl_23.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	/* Maximum number of fingers supported */
	fhandler->num_of_data_points = min(ctrl_23.max_reported_objects,
			(unsigned char)F12_FINGERS_TO_SUPPORT);

	rmi4_data->num_of_fingers = fhandler->num_of_data_points;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 7,
			&size_of_query8,
			sizeof(size_of_query8));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 8,
			query_8.data,
			size_of_query8);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	/* Determine the presence of the Data0 register */
	extra_data->data1_offset = query_8.data0_is_present;

	if ((size_of_query8 >= 3) && (query_8.data15_is_present)) {
		extra_data->data15_offset = query_8.data0_is_present +
			query_8.data1_is_present +
			query_8.data2_is_present +
			query_8.data3_is_present +
			query_8.data4_is_present +
			query_8.data5_is_present +
			query_8.data6_is_present +
			query_8.data7_is_present +
			query_8.data8_is_present +
			query_8.data9_is_present +
			query_8.data10_is_present +
			query_8.data11_is_present +
			query_8.data12_is_present +
			query_8.data13_is_present +
			query_8.data14_is_present;
		extra_data->data15_size = (rmi4_data->num_of_fingers + 7) / 8;
	} else {
		extra_data->data15_size = 0;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base + 10,
			query_10.data,
			sizeof(query_10.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	if (query_10.glove_mode_feature)
		rmi4_data->has_glove_mode = true;
	else
		rmi4_data->has_glove_mode = false;

	rmi4_data->report_enable = RPT_DEFAULT;
#ifdef REPORT_2D_Z
	rmi4_data->report_enable |= RPT_Z;
#endif
#ifdef REPORT_2D_W
	rmi4_data->report_enable |= (RPT_WX | RPT_WY);
#endif

/*
 * Set f12 control register address.
 * control register address : cntrol base register address + offset
 */
	rmi4_data->f12_ctrl11_addr = fhandler->full_addr.ctrl_base + ctrl_11_offset;
	rmi4_data->f12_ctrl15_addr = fhandler->full_addr.ctrl_base + ctrl_15_offset;
	rmi4_data->f12_ctrl26_addr = fhandler->full_addr.ctrl_base + ctrl_26_offset;
	rmi4_data->f12_ctrl28_addr = fhandler->full_addr.ctrl_base + ctrl_28_offset;

#ifdef ENABLE_F12_OBJTYPE
	rmi4_data->f12_ctrl23_addr = fhandler->full_addr.ctrl_base + ctrl_23_offset;
	rmi4_data->obj_type_enable = OBJ_TYPE_FINGER | OBJ_TYPE_UNCLASSIFIED;
#endif

	if (rmi4_data->has_glove_mode) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f12_ctrl26_addr,
				&rmi4_data->feature_enable,
				sizeof(rmi4_data->feature_enable));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
			return retval;
		}
	
		retval = synaptics_rmi4_f12_set_feature(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: f12_set_feature fail[%d]\n",
					__func__, retval);
			return retval;
		}
	}

	retval = synaptics_rmi4_f12_set_report(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: f12_set_report fail[%d]\n",
				__func__, retval);
		return retval;
	}

#ifdef ENABLE_F12_OBJTYPE
	retval = synaptics_rmi4_f12_obj_type_enable(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s, Failed to set object. [%d]\n",
			__func__, retval);
		return retval;
	}
#endif

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_8_offset,
			ctrl_8.data,
			sizeof(ctrl_8.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	/* Maximum x and y */
	rmi4_data->sensor_max_x =
		((unsigned short)ctrl_8.max_x_coord_lsb << 0) |
		((unsigned short)ctrl_8.max_x_coord_msb << 8);
	rmi4_data->sensor_max_y =
		((unsigned short)ctrl_8.max_y_coord_lsb << 0) |
		((unsigned short)ctrl_8.max_y_coord_msb << 8);
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: Function %02x max x = %d max y = %d\n",
			__func__, fhandler->fn_number,
			rmi4_data->sensor_max_x,
			rmi4_data->sensor_max_y);
	if (rmi4_data->sensor_max_x <= 0 || rmi4_data->sensor_max_y <= 0) {
		rmi4_data->sensor_max_x = rmi4_data->dt_data->coords[0];
		rmi4_data->sensor_max_y = rmi4_data->dt_data->coords[1];

		dev_info(&rmi4_data->i2c_client->dev,
				"%s: Function %02x read failed, run dtsi coords. max x = %d max y = %d\n",
				__func__, fhandler->fn_number,
				rmi4_data->sensor_max_x,
				rmi4_data->sensor_max_y);
	}

	rmi4_data->num_of_rx = ctrl_8.num_of_rx;
	rmi4_data->num_of_tx = ctrl_8.num_of_tx;
	rmi4_data->num_of_node = ctrl_8.num_of_rx * ctrl_8.num_of_tx;

	rmi4_data->max_touch_width = max(rmi4_data->num_of_rx,
			rmi4_data->num_of_tx);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base + ctrl_9_offset,
			ctrl_9.data,
			sizeof(ctrl_9.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	rmi4_data->touch_threshold = (int)ctrl_9.touch_threshold;
	rmi4_data->gloved_sensitivity = (int)ctrl_9.gloved_finger;
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: %02x num_Rx:%d num_Tx:%d, node:%d, threshold:%d, gloved sensitivity:%x\n",
			__func__, fhandler->fn_number,
			rmi4_data->num_of_rx, rmi4_data->num_of_tx,
			rmi4_data->num_of_node, rmi4_data->touch_threshold,
			rmi4_data->gloved_sensitivity);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f12_ctrl11_addr,
			ctrl_11.data,
			sizeof(ctrl_11.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	dev_info(&rmi4_data->i2c_client->dev,
			"%s: jitter filter strenth = %d, glove jitter filter strenght = %d\n",
			__func__, (int)ctrl_11.jitter_filter_strength,
			(int)ctrl_11.gloved_finger_jitter_filter_strength);

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
				intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	/* Allocate memory for finger data storage space */
	fhandler->data_size = rmi4_data->num_of_fingers * size_of_2d_data;
	fhandler->data = kzalloc(fhandler->data_size, GFP_KERNEL);

	return retval;
}

static int synaptics_rmi4_f1a_alloc_mem(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	struct synaptics_rmi4_f1a_handle *f1a;

	f1a = kzalloc(sizeof(*f1a), GFP_KERNEL);
	if (!f1a) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for function handle\n",
				__func__);
		return -ENOMEM;
	}

	fhandler->data = (void *)f1a;
	fhandler->extra = NULL;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			f1a->button_query.data,
			sizeof(f1a->button_query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read query registers\n",
				__func__);
		return retval;
	}

	f1a->max_count = f1a->button_query.max_button_count + 1;

	f1a->button_control.txrx_map = kzalloc(f1a->max_count * 2, GFP_KERNEL);
	if (!f1a->button_control.txrx_map) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for tx rx mapping\n",
				__func__);
		return -ENOMEM;
	}

	f1a->button_bitmask_size = (f1a->max_count + 7) / 8;

	f1a->button_data_buffer = kcalloc(f1a->button_bitmask_size,
			sizeof(*(f1a->button_data_buffer)), GFP_KERNEL);
	if (!f1a->button_data_buffer) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for data buffer\n",
				__func__);
		return -ENOMEM;
	}

	f1a->button_map = kcalloc(f1a->max_count,
			sizeof(*(f1a->button_map)), GFP_KERNEL);
	if (!f1a->button_map) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for button map\n",
				__func__);
		return -ENOMEM;
	}

	return 0;
}
#if !defined(CONFIG_SEC_RUBENS_PROJECT)
static int synaptics_rmi4_f1a_button_map(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	unsigned char ii;
	unsigned char mapping_offset = 0;
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;

	mapping_offset = f1a->button_query.has_general_control +
		f1a->button_query.has_interrupt_enable +
		f1a->button_query.has_multibutton_select;

	if (f1a->button_query.has_tx_rx_map) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				fhandler->full_addr.ctrl_base + mapping_offset,
				f1a->button_control.txrx_map,
				sizeof(f1a->button_control.txrx_map));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to read tx rx mapping\n",
					__func__);
			return retval;
		}

		rmi4_data->button_txrx_mapping = f1a->button_control.txrx_map;
	}

	if (!rmi4_data->dt_data->f1a_button_map) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: f1a_button_map is NULL in dtsi file\n",
				__func__);
		return -ENODEV;
	} else if (!rmi4_data->dt_data->f1a_button_map->map) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Button map is missing in dtsi file\n",
				__func__);
		return -ENODEV;
	} else {
		if (rmi4_data->dt_data->f1a_button_map->nbuttons != f1a->max_count) {
			f1a->valid_button_count = min(f1a->max_count,
					rmi4_data->dt_data->f1a_button_map->nbuttons);
		} else {
			f1a->valid_button_count = f1a->max_count;
		}

		for (ii = 0; ii < f1a->valid_button_count; ii++)
			f1a->button_map[ii] = rmi4_data->dt_data->f1a_button_map->map[ii];
	}

	return 0;
}
#endif
static void synaptics_rmi4_f1a_kfree(struct synaptics_rmi4_fn *fhandler)
{
	struct synaptics_rmi4_f1a_handle *f1a = fhandler->data;

	if (f1a) {
		kfree(f1a->button_control.txrx_map);
		kfree(f1a->button_data_buffer);
		kfree(f1a->button_map);
		kfree(f1a);
		fhandler->data = NULL;
	}

	return;
}

static int synaptics_rmi4_f1a_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) +
				intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	retval = synaptics_rmi4_f1a_alloc_mem(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;
#if !defined(CONFIG_SEC_RUBENS_PROJECT)
	retval = synaptics_rmi4_f1a_button_map(rmi4_data, fhandler);
	if (retval < 0)
		goto error_exit;

	rmi4_data->button_0d_enabled = 1;
#endif
	return 0;

error_exit:
	synaptics_rmi4_f1a_kfree(fhandler);

	return retval;
}

int synaptics_rmi4_tsp_read_test_result(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char data;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f34_ctrl_base_addr,
			&data,
			sizeof(data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
				__func__, retval);
		return retval;
	}

	dev_info(&rmi4_data->i2c_client->dev, "%s: test_result : [%x]\n",
			__func__, data >> 4);

	return data >> 4;
}

static int synaptics_rmi4_f34_read_version(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler)
{
	int retval;
	struct synaptics_rmi4_f34_ctrl_3 ctrl_3;

	/* Read bootloader version */
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			rmi4_data->bootloader_id,
			sizeof(rmi4_data->bootloader_id));
	if (retval < 0) {
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: Failed to read. error = %d\n",
			__func__, retval);
		return retval;
	}

	rmi4_data->f34_ctrl_base_addr = fhandler->full_addr.ctrl_base;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.ctrl_base,
			ctrl_3.data,
			sizeof(ctrl_3.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
				__func__, retval);
		return retval;
	}

	dev_info(&rmi4_data->i2c_client->dev, "%s: [IC] test_result[%x][date, revision, version] [%02d/%02d, 0x%02X, 0x%02X], Bootlader : v%s\n",
			__func__, ctrl_3.fw_release_month >> 4, ctrl_3.fw_release_month & 0xF, ctrl_3.fw_release_date,
			ctrl_3.fw_release_revision, ctrl_3.fw_release_version, rmi4_data->bootloader_id);

	rmi4_data->fw_release_date_of_ic =
		(((ctrl_3.fw_release_month & 0xF) << 8) | ctrl_3.fw_release_date);
	rmi4_data->ic_revision_of_ic = ctrl_3.fw_release_revision;
	rmi4_data->fw_version_of_ic = ctrl_3.fw_release_version;

	return retval;
}

static int synaptics_rmi4_f34_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	retval = synaptics_rmi4_f34_read_version(rmi4_data, fhandler);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
				__func__, retval);
		return retval;
	}

	fhandler->data = NULL;

	return retval;
}

#ifdef PROXIMITY
#ifdef USE_HOVER_REZERO
static void synaptics_rmi4_rezero_work(struct work_struct *work)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data =
			container_of(work, struct synaptics_rmi4_data,
			rezero_work.work);

	unsigned char custom_rezero;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->general_control_addr,
			&custom_rezero, sizeof(custom_rezero));

	custom_rezero |= 0x10;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->general_control_addr,
			&custom_rezero, sizeof(custom_rezero));

	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: fail to write custom rezero\n",
			__func__);
		return;
	}
	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);
}
#endif

#ifdef USE_EDGE_EXCLUSION
int synaptics_rmi4_f51_grip_edge_exclusion_rx(struct synaptics_rmi4_data *rmi4_data, bool enables)
{
	int retval;
	unsigned char grip_edge = 0;

	if (!rmi4_data->f51_handle) {
		printk(KERN_ERR "%s: f51 is not set!\n", __func__);
		return -ENODEV;
	}

	if (enables)
		grip_edge = 0x0;
	else
		grip_edge = 0x10;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->proximity_enables_addr + F51_GPIP_EDGE_EXCLUSION_RX_OFFSET,
			&grip_edge,
			sizeof(grip_edge));
	if (retval <= 0)
		dev_err(&rmi4_data->i2c_client->dev, "%s: fail to set grip_edge[%X][ret:%d]\n",
				__func__, grip_edge, retval);
	else
		dev_info(&rmi4_data->i2c_client->dev, "%s: value is 0x%02x\n",
				__func__, grip_edge);

	return retval;
}
#endif

int synaptics_proximity_no_sleep_set(struct synaptics_rmi4_data *rmi4_data, bool enables)
{
	int retval;
	unsigned char no_sleep = 0;

	if (!rmi4_data->f51_handle) {
		printk(KERN_ERR "%s: f51 is not set!\n", __func__);
		return -ENODEV;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&no_sleep,
			sizeof(no_sleep));

	if (retval <= 0)
		dev_err(&rmi4_data->i2c_client->dev, "%s: fail to read no_sleep[ret:%d]\n",
				__func__, retval);

	if (enables)
		no_sleep |= NO_SLEEP_ON;
	else
		no_sleep &= ~(NO_SLEEP_ON);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&no_sleep,
			sizeof(no_sleep));
	if (retval <= 0)
		dev_err(&rmi4_data->i2c_client->dev, "%s: fail to set no_sleep[%X][ret:%d]\n",
				__func__, no_sleep, retval);

	return retval;
}
EXPORT_SYMBOL(synaptics_proximity_no_sleep_set);

static int synaptics_rmi4_f51_set_enables(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	if (!rmi4_data->f51_handle) {
		printk(KERN_ERR "%s: f51 is not set!\n", __func__);
		return 0;
	}

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->proximity_enables_addr,
			&rmi4_data->f51_handle->proximity_enables,
			sizeof(rmi4_data->f51_handle->proximity_enables));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to write proximity enable\n",
				__func__);
		return retval;
	}
#ifdef USE_HOVER_REZERO
	if (rmi4_data->f51_handle->proximity_enables & FINGER_HOVER_EN)
		schedule_delayed_work(&rmi4_data->rezero_work,
						msecs_to_jiffies(300));
#endif
	return 0;
}

/* mode ? read register : write register */
int synaptics_rmi4_set_custom_ctrl_register(struct synaptics_rmi4_data *rmi4_data,
					bool mode, unsigned short address,
					int length, unsigned char *value)
{
	int retval = 0, ii = 0;
	unsigned char data[10] = {0, };
	char temp[70] = {0, };
	char t_temp[7] = {0, };

	if (mode) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				address, data, length);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: do not read 0x%04X\n",
				__func__, address);
			return -EIO;
		}
		memcpy(value, data, length);

		snprintf(temp, 1, ":");
		while (ii < length) {
			snprintf(t_temp, 7, "0x%X, ", data[ii]);
			strcat(temp, t_temp);
			ii++;
		}

		dev_info(&rmi4_data->i2c_client->dev, "%s: [R]0x%04X, length:%d, data: %s\n",
			__func__, address, length, temp);
	} else {
		retval = synaptics_rmi4_i2c_write(rmi4_data,
				address, value, length);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s: do not write 0x%X ~ %d byte\n",
				__func__, address, length);
		} else {

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					address, data, length);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s: do not read 0x%04X\n",
						__func__, address);
				return -EIO;
			}

			snprintf(temp, 1, ":");
			while (ii < length) {
				snprintf(t_temp, 7, "0x%X, ", data[ii]);
				strcat(temp, t_temp);
				ii++;
			}
			dev_info(&rmi4_data->i2c_client->dev, "%s: [W]0x%04X, length:%d, data: %s\n",
					__func__, address, length, temp);
		}

	}

	return retval;
}

static int synaptics_rmi4_f51_init(struct synaptics_rmi4_data *rmi4_data,
		struct synaptics_rmi4_fn *fhandler,
		struct synaptics_rmi4_fn_desc *fd,
		unsigned int intr_count)
{
	int retval;
	unsigned char ii;
	unsigned short intr_offset;
	unsigned short data_addr_offset;
	struct synaptics_rmi4_f51_data *data;
	struct synaptics_rmi4_f51_query query;

	fhandler->fn_number = fd->fn_number;
	fhandler->num_of_data_sources = fd->intr_src_count;

	fhandler->intr_reg_num = (intr_count + 7) / 8;
	if (fhandler->intr_reg_num != 0)
		fhandler->intr_reg_num -= 1;

	/* Set an enable bit for each data source */
	intr_offset = intr_count % 8;
	fhandler->intr_mask = 0;
	for (ii = intr_offset;
			ii < ((fd->intr_src_count & MASK_3BIT) + intr_offset);
			ii++)
		fhandler->intr_mask |= 1 << ii;

	fhandler->data_size = sizeof(*data);
	data = kzalloc(fhandler->data_size, GFP_KERNEL);
	if (!data) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to alloc fhandler data\n",
					__func__);
			return -ENOMEM;
	}

	fhandler->data = (void *)data;
	fhandler->extra = NULL;

	rmi4_data->f51_handle = kzalloc(sizeof(struct synaptics_rmi4_f51_handle), GFP_KERNEL);
	if (!rmi4_data->f51_handle) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to alloc f51_handler data\n",
					__func__);
			return -ENOMEM;
	}

	if (rmi4_data->dt_data->surface_only)
		rmi4_data->f51_handle->proximity_enables = 0;
	else
		rmi4_data->f51_handle->proximity_enables = AIR_SWIPE_EN | SLEEP_PROXIMITY;

	rmi4_data->f51_handle->general_control = F51_GENERAL_CONTROL;

	rmi4_data->f51_handle->proximity_enables_addr = fhandler->full_addr.ctrl_base +
		F51_PROXIMITY_ENABLES_OFFSET;
	rmi4_data->f51_handle->general_control_addr = fhandler->full_addr.ctrl_base +
		F51_GENERAL_CONTROL_OFFSET;

	rmi4_data->f51_handle->general_control2_addr = fhandler->full_addr.ctrl_base +
		F51_GENERAL_CONTROL2_OFFSET;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			fhandler->full_addr.query_base,
			query.data,
			sizeof(query.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	dev_err(&rmi4_data->i2c_client->dev, "%s: f51 query feature feature1:%X, feature2:%X\n",
			__func__, query.features, query.side_touch_feature);

	data_addr_offset = F51_DATA_RESERVED_SIZE;

	if (query.features & HAS_FINGER_HOVER)
		data_addr_offset += F51_DATA_1_SIZE;

	if (query.features & HAS_HOVER_PINCH)
		data_addr_offset += F51_DATA_2_SIZE;

	if (query.features & (HAS_AIR_SWIPE | HAS_LARGE_OBJ))
		data_addr_offset += F51_DATA_3_SIZE;

	if (query.side_touch_feature & HAS_SIDE_BUTTONS) {
		rmi4_data->f51_handle->side_button_data_addr = fhandler->full_addr.data_base + data_addr_offset;
		data_addr_offset += F51_DATA_4_SIZE;
	}

	if (query.side_touch_feature & HAS_CAMERA_GRIP_DETECTION)
		data_addr_offset += F51_DATA_5_SIZE;

	if ((query.features & HAS_EDGE_SWIPE) || (query.side_touch_feature & HAS_SIDE_BUTTONS)) {
		/* detection_flag2 address */
		rmi4_data->f51_handle->detection_flag2_addr = fhandler->full_addr.data_base + data_addr_offset;
		data_addr_offset += F51_DATA_6_SIZE;
	}

	if (query.features & HAS_EDGE_SWIPE) {
		rmi4_data->f51_handle->edge_swipe_data_addr = fhandler->full_addr.data_base + data_addr_offset;
#ifndef USE_F51_OFFSET_CALCULATE
		if (strncmp(rmi4_data->dt_data->project, "PSLTE", 5) == 0)
			rmi4_data->f51_handle->edge_swipe_data_addr =
					fhandler->full_addr.data_base;
		else
			rmi4_data->f51_handle->edge_swipe_data_addr =
					fhandler->full_addr.data_base + EDGE_SWIPE_DATA_OFFSET;
#endif
		rmi4_data->has_edge_swipe = true;
		rmi4_data->f51_handle->general_control |= EDGE_SWIPE_EN;
	} else {
		rmi4_data->has_edge_swipe = false;
	}

	if (query.side_touch_feature & HAS_SIDE_BUTTONS)
		rmi4_data->has_side_buttons = true;
	else
		rmi4_data->has_side_buttons = false;

	if (!rmi4_data->dt_data->surface_only) {
		retval = synaptics_rmi4_f51_set_enables(rmi4_data);
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: f51_set_enables fail[%d]\n",
					__func__, retval);
			return retval;
		}
	}

	dev_info(&rmi4_data->i2c_client->dev,
			"%s: edge_swipe: %04X, detection_flag2 : %04X\n",
			__func__, rmi4_data->f51_handle->edge_swipe_data_addr,
			rmi4_data->f51_handle->detection_flag2_addr);

	rmi4_data->f51_ctrl_base_addr = fhandler->full_addr.ctrl_base;

#ifdef SIDE_TOUCH
		rmi4_data->f51_handle->sidekey_threshold_addr = fhandler->full_addr.ctrl_base + F51_CUSTOM_CTRL78_OFFSET;
#endif
#ifdef USE_STYLUS
		rmi4_data->f51_handle->stylus_enable_addr = fhandler->full_addr.ctrl_base + F51_CUSTOM_CTRL87_OFFSET;
#endif
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: sidekey_threshold addr : %04X, stylus_enable_addr : %04X\n",
			__func__, rmi4_data->f51_handle->sidekey_threshold_addr,
			rmi4_data->f51_handle->stylus_enable_addr);

	return 0;
}

int synaptics_rmi4_proximity_enables(struct synaptics_rmi4_data *rmi4_data, unsigned char enables)
{
	int retval;

	if (!rmi4_data->f51_handle) {
		printk(KERN_ERR "%s: f51 is not set!\n", __func__);
		return -ENODEV;
	}

#ifdef HAND_GRIP_MODE
	if (rmi4_data->f51_handle->rmi4_data->hand_grip_mode)
		rmi4_data->f51_handle->proximity_enables |= ENABLE_HANDGRIP_RECOG;
	else
#endif
		rmi4_data->f51_handle->proximity_enables &= ~(ENABLE_HANDGRIP_RECOG);

	if (enables) {
		rmi4_data->f51_handle->proximity_enables |= FINGER_HOVER_EN;
		rmi4_data->f51_handle->proximity_enables &= ~(SLEEP_PROXIMITY);
	} else {
		rmi4_data->f51_handle->proximity_enables |= SLEEP_PROXIMITY;
		rmi4_data->f51_handle->proximity_enables &= ~(FINGER_HOVER_EN);
	}

	retval = synaptics_rmi4_f51_set_enables(rmi4_data);
	if (retval < 0)
		return retval;

	return 0;
}
EXPORT_SYMBOL(synaptics_rmi4_proximity_enables);
#endif

static int synaptics_rmi4_check_status(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	int timeout = CHECK_STATUS_TIMEOUT_MS;
	unsigned char intr_status;
	struct synaptics_rmi4_f01_device_status status;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr,
			status.data,
			sizeof(status.data));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
		return retval;
	}

	dev_info(&rmi4_data->i2c_client->dev, "%s: Device status[0x%02x] status.code[%d]\n",
			__func__, status.data[0], status.status_code);

	while (status.status_code == STATUS_CRC_IN_PROGRESS) {
		if (timeout > 0)
			msleep(20);
		else
			return -1;

		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f01_data_base_addr,
				status.data,
				sizeof(status.data));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
				__func__, __LINE__, retval);
			return retval;
		}

		timeout -= 20;
	}

	if (status.flash_prog == 1) {
		rmi4_data->firmware_cracked = rmi4_data->flash_prog_mode = true;
		dev_info(&rmi4_data->i2c_client->dev, "%s: In flash prog mode, status = 0x%02x\n",
				__func__, status.status_code);
	} else {
		rmi4_data->flash_prog_mode = false;
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_data_base_addr + 1,
			&intr_status,
			sizeof(intr_status));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to read interrupt status\n",
				__func__);
		return retval;
	}

	return 0;
}

static void synaptics_rmi4_set_configured(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to set configured\n",
				__func__);
		return;
	}

	device_ctrl |= CONFIGURED;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to set configured\n",
				__func__);
	}

	return;
}

static int synaptics_rmi4_alloc_fh(struct synaptics_rmi4_fn **fhandler,
		struct synaptics_rmi4_fn_desc *rmi_fd, int page_number)
{
	*fhandler = kzalloc(sizeof(**fhandler), GFP_KERNEL);
	if (!(*fhandler))
		return -ENOMEM;

	(*fhandler)->full_addr.data_base =
		(rmi_fd->data_base_addr |
		 (page_number << 8));
	(*fhandler)->full_addr.ctrl_base =
		(rmi_fd->ctrl_base_addr |
		 (page_number << 8));
	(*fhandler)->full_addr.cmd_base =
		(rmi_fd->cmd_base_addr |
		 (page_number << 8));
	(*fhandler)->full_addr.query_base =
		(rmi_fd->query_base_addr |
		 (page_number << 8));

	return 0;
}

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
static void synaptics_rmi_select_ta_mode(struct synaptics_rmi4_data *rmi4_data)
{
	/* ta_con_mode true : I2C (RMI)
	 * ta_con_mode false : INT (GPIO)
	 */

	if (system_rev >= TA_CON_REVISION)
		rmi4_data->ta_con_mode = false;
	else
		rmi4_data->ta_con_mode = true;
}
#endif

/**
 * synaptics_rmi4_query_device()
 *
 * Called by synaptics_rmi4_probe().
 *
 * This funtion scans the page description table, records the offsets
 * to the register types of Function $01, sets up the function handlers
 * for Function $11 and Function $12, determines the number of interrupt
 * sources from the sensor, adds valid Functions with data inputs to the
 * Function linked list, parses information from the query registers of
 * Function $01, and enables the interrupt sources from the valid Functions
 * with data inputs.
 */
static int synaptics_rmi4_query_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii = 0;
	unsigned char page_number;
	unsigned char intr_count = 0;
	unsigned char f01_query[F01_STD_QUERY_LEN];
#ifdef CHECK_BASE_FIRMWARE
	unsigned char f01_pr_number[4];
#endif
	unsigned char f01_package_id[F01_PACKAGE_ID_LEN];
	unsigned short pdt_entry_addr;
	unsigned short intr_addr;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	INIT_LIST_HEAD(&rmi->support_fn_list);

	/* Scan the page description tables of the pages to service */
	for (page_number = 0; page_number < PAGES_TO_SERVICE; page_number++) {
		for (pdt_entry_addr = PDT_START; pdt_entry_addr > PDT_END;
				pdt_entry_addr -= PDT_ENTRY_SIZE) {
			pdt_entry_addr |= (page_number << 8);

			retval = synaptics_rmi4_i2c_read(rmi4_data,
					pdt_entry_addr,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s: read fail[%d]\n",
							__func__, retval);
				return retval;
			}

			fhandler = NULL;

			if (rmi_fd.fn_number == 0) {
				dev_dbg(&rmi4_data->i2c_client->dev,
						"%s: Reached end of PDT\n",
						__func__);
				break;
			}

			/* Display function description infomation */
			dev_info(&rmi4_data->i2c_client->dev, "%s: F%02x found (page %d): INT_SRC[%02X] BASE_ADDRS[%02X,%02X,%02X,%02x]\n",
					__func__, rmi_fd.fn_number, page_number,
					rmi_fd.intr_src_count, rmi_fd.data_base_addr,
					rmi_fd.ctrl_base_addr, rmi_fd.cmd_base_addr,
					rmi_fd.query_base_addr);

			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				rmi4_data->f01_query_base_addr =
					rmi_fd.query_base_addr;
				rmi4_data->f01_ctrl_base_addr =
					rmi_fd.ctrl_base_addr;
				rmi4_data->f01_data_base_addr =
					rmi_fd.data_base_addr;
				rmi4_data->f01_cmd_base_addr =
					rmi_fd.cmd_base_addr;

				retval = synaptics_rmi4_check_status(rmi4_data);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to check status\n",
							__func__);
					return retval;
				}
				if (rmi4_data->flash_prog_mode)
					goto flash_prog_mode;
				break;
			case SYNAPTICS_RMI4_F11:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}
				retval = synaptics_rmi4_f11_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f11_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;
			case SYNAPTICS_RMI4_F12:
				if (rmi_fd.intr_src_count == 0)
					break;
				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}
				retval = synaptics_rmi4_f12_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f12_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;
			case SYNAPTICS_RMI4_F1A:
				if (rmi_fd.intr_src_count == 0)
					break;
				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}
				retval = synaptics_rmi4_f1a_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f1a_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;
			case SYNAPTICS_RMI4_F34:
				if (rmi_fd.intr_src_count == 0)
					break;

				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}

				retval = synaptics_rmi4_f34_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f34_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;

#ifdef PROXIMITY
			case SYNAPTICS_RMI4_F51:
				if (rmi_fd.intr_src_count == 0)
					break;
				retval = synaptics_rmi4_alloc_fh(&fhandler,
						&rmi_fd, page_number);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to alloc for F%d\n",
							__func__,
							rmi_fd.fn_number);
					return retval;
				}
				retval = synaptics_rmi4_f51_init(rmi4_data,
						fhandler, &rmi_fd, intr_count);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f51_init fail[%d]\n",
							__func__, retval);
					kfree(fhandler);
					return retval;
				}
				break;
#endif
			}

			/* Accumulate the interrupt count */
			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);

			if (fhandler && rmi_fd.intr_src_count) {
				list_add_tail(&fhandler->link,
						&rmi->support_fn_list);
			}
		}
	}

flash_prog_mode:
	rmi4_data->num_of_intr_regs = (intr_count + 7) / 8;
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: Number of interrupt registers = %d sum of intr_count = %d\n",
			__func__, rmi4_data->num_of_intr_regs, intr_count);

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr,
			f01_query,
			sizeof(f01_query));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
		return retval;
	}

	/* RMI Version 4.0 currently supported */
	rmi->version_major = 4;
	rmi->version_minor = 0;

	rmi->manufacturer_id = f01_query[0];
	rmi->product_props = f01_query[1];
	rmi->product_info[0] = f01_query[2] & MASK_7BIT;
	rmi->product_info[1] = f01_query[3] & MASK_7BIT;
	rmi->date_code[0] = f01_query[4] & MASK_5BIT;
	rmi->date_code[1] = f01_query[5] & MASK_4BIT;
	rmi->date_code[2] = f01_query[6] & MASK_5BIT;
	rmi->tester_id = ((f01_query[7] & MASK_7BIT) << 8) |
		(f01_query[8] & MASK_7BIT);
	rmi->serial_number = ((f01_query[9] & MASK_7BIT) << 8) |
		(f01_query[10] & MASK_7BIT);
	memcpy(rmi->product_id_string, &f01_query[11], 10);

#ifdef CHECK_BASE_FIRMWARE
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr + 18,
			f01_pr_number,
			sizeof(f01_pr_number));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
		return retval;
	}
	memcpy(&rmi->pr_number, f01_pr_number, 3);
#endif

	if (strncmp(rmi->product_id_string + 1, "5000", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5000;
	else if (strncmp(rmi->product_id_string + 1, "5050", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5050;
	else if (strncmp(rmi->product_id_string + 1, "5100", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5100;
	else if (strncmp(rmi->product_id_string + 1, "5700", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5700;
	else if (strncmp(rmi->product_id_string + 1, "5708", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5708;
	else if (strncmp(rmi->product_id_string + 1, "5707", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5707;
	else if (strncmp(rmi->product_id_string + 1, "5006", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5006;
	else if (strncmp(rmi->product_id_string + 1, "5710", 4) == 0)
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_S5710;
	else
		rmi4_data->ic_version = SYNAPTICS_PRODUCT_ID_NONE;

/* below code is only S5000 IC temporary code. will be removed..*/
	if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5000)
		rmi4_data->ic_revision_of_ic = 0xB0;
#if !defined(CONFIG_SEC_HESTIA_PROJECT)
	if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) {
		if ((strncmp(rmi->product_id_string + 6, "A1", 2) == 0) ||
			(strncmp(rmi->product_id_string + 6, "a1", 2) == 0)) {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_A1;
		} else if ((strncmp(rmi->product_id_string + 6, "A2", 2) == 0) ||
			(strncmp(rmi->product_id_string + 6, "a2", 2) == 0)) {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_A2;
		} else if ((strncmp(rmi->product_id_string + 6, "A3", 2) == 0) ||
			(strncmp(rmi->product_id_string + 6, "a3", 2) == 0)) {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_A3;
		} else if ((strncmp(rmi->product_id_string + 6, "B0", 2) == 0) ||
			(strncmp(rmi->product_id_string + 6, "b0", 2) == 0)) {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_B0;
		} else if ((strncmp(rmi->product_id_string + 6, "AF", 2) == 0) ||
			(strncmp(rmi->product_id_string + 6, "af", 2) == 0)) {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_AF;
		} else if ((strncmp(rmi->product_id_string + 6, "BF", 2) == 0) ||
			(strncmp(rmi->product_id_string + 6, "bf", 2) == 0)) {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_BF;
		} else {
			rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_NONE;
		}
	}
#endif
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			0x31, f01_package_id,
			sizeof(f01_package_id));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
		return retval;
	}

	rmi->package_id = (f01_package_id[1] << 8) | f01_package_id[0];
	rmi->package_rev = (f01_package_id[3] << 8) | f01_package_id[2];
	dev_info(&rmi4_data->i2c_client->dev,
			"%s: [%s] package_id is %d, package_rev is %d\n",
			__func__, rmi->product_id_string,
			rmi->package_id, rmi->package_rev);

	if (rmi->manufacturer_id != 1) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Non-Synaptics device found, manufacturer ID = %d\n",
				__func__, rmi->manufacturer_id);
	}

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_query_base_addr + F01_BUID_ID_OFFSET,
			rmi->build_id,
			sizeof(rmi->build_id));
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev, "%s:%4d read fail[%d]\n",
					__func__, __LINE__, retval);
		return retval;
	}

	memset(rmi4_data->intr_mask, 0x00, sizeof(rmi4_data->intr_mask));

	/*
	 * Map out the interrupt bit masks for the interrupt sources
	 * from the registered function handlers.
	 */
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->num_of_data_sources) {
				rmi4_data->intr_mask[fhandler->intr_reg_num] |=
					fhandler->intr_mask;
			}

			/* To display each fhandler data */
			dev_info(&rmi4_data->i2c_client->dev,
				"%s: F%02x : NUM_SOURCE[%02X] NUM_INT_REG[%02X] INT_MASK[%02X]\n",
					__func__, fhandler->fn_number,
					fhandler->num_of_data_sources, fhandler->intr_reg_num,
					fhandler->intr_mask);
		}
	}

	/* Enable the interrupt sources */
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			dev_info(&rmi4_data->i2c_client->dev,
					"%s: Interrupt enable mask %d = 0x%02x\n",
					__func__, ii, rmi4_data->intr_mask[ii]);
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s:%4d write fail[%d]\n",
					__func__, __LINE__, retval);
				return retval;
			}
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	return 0;
}

static void synaptics_rmi4_release_support_fn(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_fn *fhandler, *n;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	if (list_empty(&rmi->support_fn_list)) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: support_fn_list is empty\n",
				__func__);
#ifdef PROXIMITY
		if (rmi4_data->f51_handle)
			kfree(rmi4_data->f51_handle);
			rmi4_data->f51_handle = NULL;
#endif
		return;
	}

	list_for_each_entry_safe(fhandler, n, &rmi->support_fn_list, link) {
		dev_dbg(&rmi4_data->i2c_client->dev, "%s: fn_number = %x\n",
				__func__, fhandler->fn_number);
		if (fhandler->fn_number == SYNAPTICS_RMI4_F1A)
			synaptics_rmi4_f1a_kfree(fhandler);
		else
			kfree(fhandler->data);

		kfree(fhandler);
	}
}


#ifdef SIDE_TOUCH
static void synaptics_rmi4_set_side_btns(struct synaptics_rmi4_data *rmi4_data)
{
	set_bit(KEY_SIDE_TOUCH_0, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_1, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_2, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_3, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_4, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_5, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_6, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_TOUCH_7, rmi4_data->input_dev->keybit);
	set_bit(KEY_SIDE_CAMERA_DETECTED, rmi4_data->input_dev->keybit);
	return;
}
#endif

static void synaptics_rmi4_set_input_data(struct synaptics_rmi4_data *rmi4_data)
{
	int temp;
	unsigned char ii;
	struct synaptics_rmi4_f1a_handle *f1a;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

#ifdef GLOVE_MODE
	input_set_capability(rmi4_data->input_dev, EV_SW, SW_GLOVE);
#endif
#ifdef HAND_GRIP_MODE
	input_set_capability(rmi4_data->input_dev, EV_SW, SW_LEFT_HAND);
	input_set_capability(rmi4_data->input_dev, EV_SW, SW_RIGHT_HAND);
	input_set_capability(rmi4_data->input_dev, EV_SW, SW_BOTH_HAND);
#endif

	set_bit(EV_SYN, rmi4_data->input_dev->evbit);
	set_bit(EV_KEY, rmi4_data->input_dev->evbit);
	set_bit(EV_ABS, rmi4_data->input_dev->evbit);
	set_bit(BTN_TOUCH, rmi4_data->input_dev->keybit);
	set_bit(BTN_TOOL_FINGER, rmi4_data->input_dev->keybit);
#if defined(TOUCHKEY_ENABLE)
	set_bit(EV_LED, rmi4_data->input_dev->evbit);
	set_bit(LED_MISC, rmi4_data->input_dev->ledbit);
#endif
#ifdef INPUT_PROP_DIRECT
	set_bit(INPUT_PROP_DIRECT, rmi4_data->input_dev->propbit);
#endif

	if (rmi4_data->dt_data->swap_axes) {
		temp = rmi4_data->sensor_max_x;
		rmi4_data->sensor_max_x = rmi4_data->sensor_max_y;
		rmi4_data->sensor_max_y = temp;
	}

	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_X, 0,
			rmi4_data->sensor_max_x, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_POSITION_Y, 0,
			rmi4_data->sensor_max_y, 0, 0);
#ifdef PROXIMITY
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_DISTANCE, 0,
			HOVER_Z_MAX, 0, 0);
#ifdef EDGE_SWIPE
#ifdef REPORT_2D_W
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MAJOR, 0,
			EDGE_SWIPE_WIDTH_MAX, 0, 0);
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOUCH_MINOR, 0,
			EDGE_SWIPE_WIDTH_MAX, 0, 0);
#endif
#ifdef USE_EDGE_SWIPE_WIDTH_MAJOR
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_WIDTH_MAJOR, 0,
			EDGE_SWIPE_WIDTH_MAX, 0, 0);
#endif
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_PALM, 0,
			EDGE_SWIPE_PALM_MAX, 0, 0);
#endif
	setup_timer(&rmi4_data->f51_finger_timer,
			synaptics_rmi4_f51_finger_timer,
			(unsigned long)rmi4_data);
#endif
#ifdef USE_STYLUS
	input_set_abs_params(rmi4_data->input_dev,
			ABS_MT_TOOL_TYPE, 0,
			MT_TOOL_MAX, 0, 0);
#endif

	input_mt_init_slots(rmi4_data->input_dev,
			rmi4_data->num_of_fingers);

	f1a = NULL;
	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F1A)
				f1a = fhandler->data;
		}
	}

	if (f1a) {
		for (ii = 0; ii < f1a->valid_button_count; ii++) {
			set_bit(f1a->button_map[ii],
					rmi4_data->input_dev->keybit);
			input_set_capability(rmi4_data->input_dev,
					EV_KEY, f1a->button_map[ii]);
		}
	}
}

static int synaptics_rmi4_set_input_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;

	rmi4_data->input_dev = input_allocate_device();
	if (rmi4_data->input_dev == NULL) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to allocate input device\n",
				__func__);
		retval = -ENOMEM;
		goto err_input_device;
	}

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
		goto err_query_device;
	}

#ifdef PROXIMITY
	if (rmi4_data->has_edge_swipe) {
		rmi4_data->max_touch_width *= EDGE_SWIPE_WIDTH_SCALING_FACTOR;
	}
#endif

	rmi4_data->input_dev->name = "sec_touchscreen";
	rmi4_data->input_dev->id.bustype = BUS_I2C;
	rmi4_data->input_dev->dev.parent = &rmi4_data->i2c_client->dev;
#ifdef USE_OPEN_CLOSE
	rmi4_data->input_dev->open = synaptics_rmi4_input_open;
	rmi4_data->input_dev->close = synaptics_rmi4_input_close;
#endif
	input_set_drvdata(rmi4_data->input_dev, rmi4_data);

	synaptics_rmi4_set_input_data(rmi4_data);

#ifdef SIDE_TOUCH
	if (rmi4_data->has_side_buttons)
		synaptics_rmi4_set_side_btns(rmi4_data);
#endif

	retval = input_register_device(rmi4_data->input_dev);
	if (retval) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register input device\n",
				__func__);
		goto err_register_input;
	}

	return 0;

err_register_input:
	input_free_device(rmi4_data->input_dev);

err_query_device:
	synaptics_rmi4_release_support_fn(rmi4_data);

err_input_device:
	return retval;
}

#ifdef SIDE_TOUCH
void synaptics_rmi4_free_sidekeys(struct synaptics_rmi4_data *rmi4_data)
{
	int ii;

	for (ii = 0; ii < 8; ii++)
		input_report_key(rmi4_data->input_dev, KEY_SIDE_TOUCH_0 + ii, 0);

	input_report_key(rmi4_data->input_dev, KEY_SIDE_CAMERA_DETECTED, 0);
	rmi4_data->sidekey_data = 0;

	input_sync(rmi4_data->input_dev);
}
#endif

int synaptics_rmi4_free_fingers(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned char ii;

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

/* if firmware is broken, occurs null pinter exception : cause is rmi4_data->f51_handle is NULL */
#ifdef PROXIMITY
	if (!rmi4_data->tsp_probe || !rmi4_data->f51_handle) {
		dev_info(&rmi4_data->i2c_client->dev, "%s: probe is not done\n", __func__);
		return 0;
	}
#endif
	for (ii = 0; ii < rmi4_data->num_of_fingers; ii++) {
		input_mt_slot(rmi4_data->input_dev, ii);
		if (rmi4_data->finger[ii].state) {
#ifdef EDGE_SWIPE
			input_report_abs(rmi4_data->input_dev,
					ABS_MT_PALM, 0);
#endif
		}
		input_mt_report_slot_state(rmi4_data->input_dev,
					MT_TOOL_FINGER, 0);
		rmi4_data->finger[ii].mcount = 0;
		rmi4_data->finger[ii].state = 0;
	}

	input_report_key(rmi4_data->input_dev,
			BTN_TOUCH, 0);
	input_report_key(rmi4_data->input_dev,
			BTN_TOOL_FINGER, 0);
#ifdef GLOVE_MODE
	input_report_switch(rmi4_data->input_dev,
			SW_GLOVE, false);
	rmi4_data->touchkey_glove_mode_status = false;
#endif

	input_sync(rmi4_data->input_dev);

	rmi4_data->fingers_on_2d = false;
#ifdef PROXIMITY
	rmi4_data->f51_finger = false;
#endif

#ifdef COMMON_INPUT_BOOSTER
	if (rmi4_data->tsp_booster->dvfs_set)
		rmi4_data->tsp_booster->dvfs_set(rmi4_data->tsp_booster, -1);
#endif

#ifdef TSP_BOOSTER
	synaptics_set_dvfs_lock(rmi4_data, -1);
#endif
#ifdef TKEY_BOOSTER
	synaptics_tkey_set_dvfs_lock(rmi4_data, 2);
#endif
#ifdef EDGE_SWIPE
	if (rmi4_data->f51_handle->surface_data.palm)
		rmi4_data->f51_handle->surface_data.palm = 0;
#endif
#ifdef USE_HOVER_REZERO
	cancel_delayed_work(&rmi4_data->rezero_work);
#endif

#ifdef SIDE_TOUCH
	synaptics_rmi4_free_sidekeys(rmi4_data);
#endif
	return 0;
}

static int synaptics_rmi4_reinit_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char ii = 0;
#ifdef PROXIMITY
	unsigned char f51_general_control;
#endif
	unsigned short intr_addr;
	struct synaptics_rmi4_fn *fhandler;
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));

	synaptics_rmi4_free_fingers(rmi4_data);

	if (!list_empty(&rmi->support_fn_list)) {
		list_for_each_entry(fhandler, &rmi->support_fn_list, link) {
			if (fhandler->fn_number == SYNAPTICS_RMI4_F12) {
#ifdef GLOVE_MODE
				retval = synaptics_rmi4_glove_mode_enables(rmi4_data);				
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: Failed to glove mode enable, error = %d\n",
							__func__, retval);
					goto exit;
				}
#else
				retval = synaptics_rmi4_f12_set_feature(rmi4_data);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
						"%s: f12 set_feature write fail[%d]\n", __func__, retval);
				}
#endif
				retval = synaptics_rmi4_f12_set_report(rmi4_data);
				if (retval < 0) {
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: f12_set_report failed[%d]\n",
							__func__, retval);
					goto exit;
				}
				break;
			}
		}
	}

#ifdef PROXIMITY
	if (rmi4_data->f51_handle) {
		retval = synaptics_rmi4_i2c_read(rmi4_data,
				rmi4_data->f51_handle->general_control_addr,
				&f51_general_control, sizeof(f51_general_control));

		/* Read the Hsync's status */
		dev_info(&rmi4_data->i2c_client->dev,
			"%s: Hsync [%s[0x%x]]\n", __func__,
			(f51_general_control & HSYNC_STATUS) ? "GD" : "NG",
			f51_general_control);

		/* general control register
		 * register bit	: 1 active	: 0 active
		 * HOST ID		: I2C(RMI)	: INT(GPIO)
		 * HSYNC status	: GOOD		: NG
		 */
#ifdef SYNAPTICS_RMI_INFORM_CHARGER
		if (rmi4_data->ta_con_mode)
			f51_general_control |= 0x80;	/* set default I2C(RMI) */
		else
			f51_general_control &= ~0x80;	/* set default INT(GPIO) */

		retval = synaptics_rmi4_i2c_write(rmi4_data,
				rmi4_data->f51_handle->general_control_addr,
				&f51_general_control, sizeof(f51_general_control));
#endif

		if (!rmi4_data->dt_data->surface_only) {
			retval = synaptics_rmi4_f51_set_enables(rmi4_data);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s: f51_set_enables fail[%d]\n",
							__func__, retval);
				goto exit;
			}
		}
	}
#endif
	for (ii = 0; ii < rmi4_data->num_of_intr_regs; ii++) {
		if (rmi4_data->intr_mask[ii] != 0x00) {
			dev_info(&rmi4_data->i2c_client->dev,
					"%s: Interrupt enable mask %d = 0x%02x\n",
					__func__, ii, rmi4_data->intr_mask[ii]);
			intr_addr = rmi4_data->f01_ctrl_base_addr + 1 + ii;
			retval = synaptics_rmi4_i2c_write(rmi4_data,
					intr_addr,
					&(rmi4_data->intr_mask[ii]),
					sizeof(rmi4_data->intr_mask[ii]));
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev, "%s: write fail[%d]\n",
					__func__, retval);
				goto exit;
			}
		}
	}

	synaptics_rmi4_set_configured(rmi4_data);

	retval = 0;

exit:
	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));
	return retval;
}

static int synaptics_rmi4_reset_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char command = 0x01;

	mutex_lock(&(rmi4_data->rmi4_reset_mutex));

	disable_irq(rmi4_data->i2c_client->irq);

	synaptics_rmi4_free_fingers(rmi4_data);

	if (!rmi4_data->stay_awake) {
		retval = synaptics_rmi4_i2c_write(rmi4_data,
				rmi4_data->f01_cmd_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to issue reset command, error = %d\n",
					__func__, retval);
			goto out;
		}

		msleep(SYNAPTICS_HW_RESET_TIME);

	} else {
		synaptics_power_ctrl(rmi4_data, false);

		msleep(30);
		synaptics_power_ctrl(rmi4_data, true);
		rmi4_data->current_page = MASK_8BIT;
		msleep(SYNAPTICS_HW_RESET_TIME);

		if (rmi4_data->firmware_cracked) {
			synaptics_rmi4_remove_exp_fn(rmi4_data);
			dev_info(&rmi4_data->i2c_client->dev,
						"%s: removed exp_func..\n",
						__func__);

			retval = synaptics_rmi4_init_exp_fn(rmi4_data);
			if (retval < 0)
				dev_err(&rmi4_data->i2c_client->dev,
						"%s: Failed to init_exp_fn at cracked fw reflash.\n",
						__func__);

			rmi4_data->firmware_cracked = false;
		}

		retval = synaptics_rmi4_f54_set_control(rmi4_data);
		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to set f54 control\n",
					__func__);
	}

	synaptics_rmi4_release_support_fn(rmi4_data);

	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to query device\n",
				__func__);
	}

	synaptics_rmi4_set_input_data(rmi4_data);

#ifdef SIDE_TOUCH
	if (rmi4_data->has_side_buttons)
		synaptics_rmi4_set_side_btns(rmi4_data);
#endif

out:
	enable_irq(rmi4_data->i2c_client->irq);
	mutex_unlock(&(rmi4_data->rmi4_reset_mutex));

	return 0;
}

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
extern void synaptics_tsp_register_callback(struct synaptics_rmi_callbacks *cb);

static void synaptics_charger_conn(struct synaptics_rmi4_data *rmi4_data,
		int ta_status)
{
	int retval;
	unsigned char charger_connected;

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&charger_connected,
			sizeof(charger_connected));
	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to set configured\n",
				__func__);
		return;
	}

	if (ta_status == 0x01 || ta_status == 0x03)
		charger_connected |= CHARGER_CONNECTED;
	else
		charger_connected &= CHARGER_DISCONNECTED;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&charger_connected,
			sizeof(charger_connected));

	if (retval < 0) {
		dev_err(&(rmi4_data->input_dev->dev),
				"%s: Failed to set configured\n",
				__func__);
	}

	dev_info(&rmi4_data->i2c_client->dev,
			"%s: device_control : %x, ta_status : %x\n",
			__func__, charger_connected, ta_status);

}

static void synaptics_ta_cb(struct synaptics_rmi_callbacks *cb, int ta_status)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(cb, struct synaptics_rmi4_data, callbacks);

	dev_info(&rmi4_data->i2c_client->dev,
			"%s: ta_status : %x\n", __func__, ta_status);

	rmi4_data->ta_status = ta_status;

	if (rmi4_data->touch_stopped || rmi4_data->doing_reflash) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: device is in suspend state or reflash.\n",
				__func__);
		return;
	}

	synaptics_charger_conn(rmi4_data, ta_status);

}
#endif

#ifdef PROXIMITY
static void synaptics_rmi4_f51_finger_timer(unsigned long data)
{
	struct synaptics_rmi4_data *rmi4_data =
		(struct synaptics_rmi4_data *)data;

	if (rmi4_data->f51_finger) {
		rmi4_data->f51_finger = false;
		mod_timer(&rmi4_data->f51_finger_timer,
				jiffies + msecs_to_jiffies(F51_FINGER_TIMEOUT));
	} else if (!rmi4_data->fingers_on_2d) {
		input_mt_slot(rmi4_data->input_dev, 0);
		input_mt_report_slot_state(rmi4_data->input_dev,
				MT_TOOL_FINGER, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOUCH, 0);
		input_report_key(rmi4_data->input_dev,
				BTN_TOOL_FINGER, 0);
		input_sync(rmi4_data->input_dev);
		if (!rmi4_data->dt_data->surface_only) {
			if (rmi4_data->f51_finger_is_hover) {
				dev_info(&rmi4_data->i2c_client->dev,
					"%s: Hover finger[OUT]\n", __func__);
				rmi4_data->f51_finger_is_hover = false;
			}
		}
	}

	return;
}
#endif

static void synaptics_rmi4_remove_exp_fn(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_exp_fn *exp_fhandler, *n;

	if (list_empty(&exp_fn_list)) {
		dev_err(&rmi4_data->i2c_client->dev, "%s: exp_fn_list empty\n",
				__func__);
		return;
	}

	list_for_each_entry_safe(exp_fhandler, n, &exp_fn_list, link) {
		if (exp_fhandler->initialized &&
				(exp_fhandler->func_remove != NULL)) {
			dev_dbg(&rmi4_data->i2c_client->dev, "%s: [%d]\n",
					__func__, exp_fhandler->fn_type);
			exp_fhandler->func_remove(rmi4_data);
		}
		list_del(&exp_fhandler->link);
		kfree(exp_fhandler);
	}
}

/*
 *	RMI_DEV = 0
 *	RMI_F54 = 1
 *	RMI_FW_UPDATER = 2
 *	RMI_DB = 3
 */
static int synaptics_rmi4_init_exp_fn(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct synaptics_rmi4_exp_fn *exp_fhandler;

	INIT_LIST_HEAD(&exp_fn_list);

	retval = rmidev_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register rmidev module\n",
				__func__);
		if (rmi4_data->firmware_cracked)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: ffirmware_cracked, skip exit routine(rmidev)\n",
					__func__);
		else
			goto error_exit;
	}

	retval = rmi4_f54_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register f54 module\n",
				__func__);
		if (rmi4_data->firmware_cracked)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: firmware_cracked, skip exit routine(f54)\n",
					__func__);
		else
			goto error_exit;
	}

	retval = rmi4_fw_update_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register fw update module\n",
				__func__);
		if (rmi4_data->firmware_cracked)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: firmware_cracked, skip exit routine(fw)\n",
					__func__);
		else
			goto error_exit;
	}

	retval = rmidb_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register rmidb module\n",
				__func__);
		if (rmi4_data->firmware_cracked)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: firmware_cracked, skip exit routine(rmidb)\n",
					__func__);
		else
			goto error_exit;

	}

#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_G
	retval = rmi_guest_module_register();
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to register guest module\n",
				__func__);
		goto error_exit;
	}
#endif

	if (list_empty(&exp_fn_list))
		return -ENODEV;

	list_for_each_entry(exp_fhandler, &exp_fn_list, link) {
		if (exp_fhandler->func_init != NULL) {
			dev_dbg(&rmi4_data->i2c_client->dev, "%s: run [%d]'s init function\n",
					__func__, exp_fhandler->fn_type);
			retval = exp_fhandler->func_init(rmi4_data);
			if (retval < 0) {
				dev_err(&rmi4_data->i2c_client->dev,
						"%s: Failed to init exp fn\n",
						__func__);
				if (rmi4_data->firmware_cracked)
					dev_err(&rmi4_data->i2c_client->dev,
							"%s: firmware_cracked, list add exp_func.\n",
							__func__);
				else
					goto error_exit;
			} else {
				exp_fhandler->initialized = true;
			}
		}
	}

	return 0;

error_exit:
	synaptics_rmi4_remove_exp_fn(rmi4_data);

	return retval;
}

/**
 * synaptics_rmi4_new_function()
 *
 * Called by other expansion Function modules in their module init and
 * module exit functions.
 *
 * This function is used by other expansion Function modules such as
 * rmi_dev to register themselves with the driver by providing their
 * initialization and removal callback function pointers so that they
 * can be inserted or removed dynamically at module init and exit times,
 * respectively.
 */
int synaptics_rmi4_new_function(enum exp_fn fn_type,
		int (*func_init)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_remove)(struct synaptics_rmi4_data *rmi4_data),
		void (*func_attn)(struct synaptics_rmi4_data *rmi4_data,
			unsigned char intr_mask))
{
	struct synaptics_rmi4_exp_fn *exp_fhandler;

	exp_fhandler = kzalloc(sizeof(*exp_fhandler), GFP_KERNEL);
	if (!exp_fhandler) {
		pr_err("%s: Failed to alloc mem for expansion function\n",
				__func__);
		return -ENOMEM;
	}
	exp_fhandler->fn_type = fn_type;
	exp_fhandler->func_init = func_init;
	exp_fhandler->func_attn = func_attn;
	exp_fhandler->func_remove = func_remove;
	list_add_tail(&exp_fhandler->link, &exp_fn_list);

	return 0;
}

void synaptics_power_ctrl(struct synaptics_rmi4_data *rmi4_data, bool enable)
{
	struct device *dev = &rmi4_data->i2c_client->dev;
	int retval, i;

	if (rmi4_data->dt_data->external_ldo > 0) {
		retval = gpio_direction_output(rmi4_data->dt_data->external_ldo, enable);
		dev_info(dev, "%s: sensor_en[3.3V][%d] is %s[%s]\n",
				__func__, rmi4_data->dt_data->external_ldo,
				enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
	}
#if defined(CONFIG_SEC_RUBENS_PROJECT)
	mdelay(5);
	if (rmi4_data->dt_data->external_ldo2 > 0) {
		retval = gpio_direction_output(rmi4_data->dt_data->external_ldo2, enable);
		dev_info(dev, "%s: sensor_en[3.3V][%d] is %s[%s]\n",
				__func__, rmi4_data->dt_data->external_ldo2,
				enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
	}
#endif
	if (enable) {
		/* Enable regulators according to the order */
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (regulator_is_enabled(rmi4_data->supplies[i].consumer)) {
				dev_err(dev, "%s: %s is already enabled\n", __func__,
					rmi4_data->supplies[i].supply);
			} else {
				retval = regulator_enable(rmi4_data->supplies[i].consumer);
				if (retval) {
					dev_err(dev, "%s: Fail to enable regulator %s[%d]\n",
						__func__, rmi4_data->supplies[i].supply, retval);
					goto err;
				}
				dev_info(dev, "%s: %s is enabled[OK]\n",
					__func__, rmi4_data->supplies[i].supply);
			}
		}

#if !defined(CONFIG_SEC_GNOTE_PROJECT) && !defined(CONFIG_SEC_CHAGALL_PROJECT) && !defined(CONFIG_SEC_HESTIA_PROJECT) && !defined(CONFIG_SEC_RUBENS_PROJECT)
		retval = qpnp_pin_config(rmi4_data->dt_data->irq_gpio,
				&synaptics_int_set[SYNAPTICS_PM_GPIO_STATE_WAKE]);
		if (retval < 0)
			dev_info(dev, "%s: wakeup int config return: %d\n", __func__, retval);
#endif

	} else {
		/* Disable regulator */
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (regulator_is_enabled(rmi4_data->supplies[i].consumer)) {
				retval = regulator_disable(rmi4_data->supplies[i].consumer);
				if (retval) {
					dev_err(dev, "%s: Fail to disable regulator %s[%d]\n",
						__func__, rmi4_data->supplies[i].supply, retval);
					goto err;
				}
				dev_info(dev, "%s: %s is disabled[OK]\n",
					__func__, rmi4_data->supplies[i].supply);
			} else {
				dev_err(dev, "%s: %s is already disabled\n", __func__,
					rmi4_data->supplies[i].supply);
			}
		}

#if !defined(CONFIG_SEC_GNOTE_PROJECT) && !defined(CONFIG_SEC_CHAGALL_PROJECT) && !defined(CONFIG_SEC_HESTIA_PROJECT) && !defined(CONFIG_SEC_RUBENS_PROJECT)
		retval = qpnp_pin_config(rmi4_data->dt_data->irq_gpio,
				&synaptics_int_set[SYNAPTICS_PM_GPIO_STATE_SLEEP]);
		if (retval < 0)
			dev_info(dev, "%s: sleep int config return: %d\n", __func__, retval);
#endif
	}

	if (rmi4_data->dt_data->reset_gpio > 0) {
		retval = gpio_direction_output(rmi4_data->dt_data->reset_gpio, enable);
		dev_info(dev, "%s: reset_gpio[%d] is %s[%s]\n",
				__func__, rmi4_data->dt_data->reset_gpio,
				enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
	}

	if (rmi4_data->dt_data->id_gpio > 0) {
		gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->id_gpio, 0,
			GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	}

	return;

err:
	if (enable) {
		enable = 0;
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (regulator_is_enabled(rmi4_data->supplies[i].consumer)) {
				retval = regulator_disable(rmi4_data->supplies[i].consumer);
				dev_err(dev, "%s: %s is disabled[%s]\n",
						__func__, rmi4_data->supplies[i].supply,
						(retval < 0) ? "NG" : "OK");
			}
		}

		if (rmi4_data->dt_data->external_ldo > 0) {
			retval = gpio_direction_output(rmi4_data->dt_data->external_ldo, enable);
			dev_err(dev, "%s: sensor_en[3.3V][%d] is %s[%s]\n",
					__func__, rmi4_data->dt_data->external_ldo,
					enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
		}
#if defined(CONFIG_SEC_RUBENS_PROJECT)
		if (rmi4_data->dt_data->external_ldo2 > 0) {
			retval = gpio_direction_output(rmi4_data->dt_data->external_ldo2, enable);
			dev_err(dev, "%s: sensor_en[3.3V][%d] is %s[%s]\n",
					__func__, rmi4_data->dt_data->external_ldo2,
					enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
		}
#endif
	} else {
		enable = 1;
		for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++) {
			if (!regulator_is_enabled(rmi4_data->supplies[i].consumer)) {
				retval = regulator_enable(rmi4_data->supplies[i].consumer);
				dev_err(dev, "%s: %s is enabled[%s]\n",
						__func__, rmi4_data->supplies[i].supply,
						(retval < 0) ? "NG" : "OK");
			}
		}

		if (rmi4_data->dt_data->external_ldo > 0) {
			retval = gpio_direction_output(rmi4_data->dt_data->external_ldo, enable);
			dev_err(dev, "%s: sensor_en[3.3V][%d] is %s[%s]\n",
					__func__, rmi4_data->dt_data->external_ldo,
					enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
		}
#if defined(CONFIG_SEC_RUBENS_PROJECT)
		if (rmi4_data->dt_data->external_ldo2 > 0) {
			retval = gpio_direction_output(rmi4_data->dt_data->external_ldo2, enable);
			dev_err(dev, "%s: sensor_en[3.3V][%d] is %s[%s]\n",
					__func__, rmi4_data->dt_data->external_ldo2,
					enable ? "enabled" : "disabled", (retval < 0) ? "NG" : "OK");
		}
#endif
	}
}

static void synaptics_get_firmware_name(struct synaptics_rmi4_data *rmi4_data)
{
	struct synaptics_rmi4_device_info *rmi = &(rmi4_data->rmi4_mod_info);

	if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5050) {
		if (strncmp(rmi4_data->dt_data->project, "H", 1) == 0) {
			if (rmi4_data->lcd_id == 0x03)
				rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
			else
				rmi4_data->firmware_name = FW_IMAGE_NAME_S5050_H;
		} else if (strncmp(rmi4_data->dt_data->project, "F", 1) == 0) {
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5050_F;
		} else {
			rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
		}
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5100) {
		if (strncmp(rmi4_data->dt_data->project, "K", 1) == 0) {
			if ((strncmp(&rmi4_data->bootloader_id[2], SYNAPTICS_IC_NEW_BOOTLOADER, 2) == 0) &&
				(strncmp(rmi->product_id_string, "s5100K", 6) == 0) &&
				gpio_get_value(rmi4_data->dt_data->extra_config[2])) {
				rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_K_A2_FHD;
				rmi4_data->ic_revision_of_ic = SYNAPTICS_IC_REVISION_A2;
			} else if (rmi4_data->ic_revision_of_ic == SYNAPTICS_IC_REVISION_A2) {
				if (gpio_get_value(rmi4_data->dt_data->extra_config[2])) {
					if ((strncmp(&rmi4_data->bootloader_id[2], SYNAPTICS_IC_NEW_BOOTLOADER, 2) == 0) &&
						(strncmp(rmi->product_id_string + 9, "F", 1) == 0)) {
						rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_K_A2_FHD;
					} else {
						rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
						dev_info(&rmi4_data->i2c_client->dev,
								"%s: product id(lockdown) is not F / IC: %s. bootloader id %s\n",
								__func__, rmi->product_id_string + 9, &rmi4_data->bootloader_id[2]);
					}
				} else {
					rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
				}
			} else if (rmi4_data->ic_revision_of_ic == SYNAPTICS_IC_REVISION_A3) {
				if (strncmp(rmi4_data->dt_data->sub_project, "0", 1) != 0) {
					if ((strncmp(rmi4_data->dt_data->sub_project, "active", 6) == 0))
							rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_K_ACTIVE;					
					else if(strncmp(rmi4_data->dt_data->sub_project, "sports", 6) == 0)
							rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_K_SPORTS;
					else
						rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
				} else {
					if ((strncmp(rmi->product_id_string, "s5100 A3 F", 10) == 0))
#if !defined(CONFIG_MACH_KLTE_KOR)
						rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_K_A3;
#else
						rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_K_A3_KOR;
#endif
					else
						rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
				}
			} else {
				rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
			}
		} else if (strncmp(rmi4_data->dt_data->project, "HESTIA", 6) == 0) {
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_HESTIA;
		} else if (strncmp(rmi4_data->dt_data->project, "PSLTE", 5) == 0) {
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5100_PSLTE;
		} else {
			rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
		}

	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5000) {
		rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5707) {
		if (strncmp(rmi4_data->dt_data->project, "Klimt", 5) == 0)
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5707_KLIMT;
		else if (strncmp(rmi4_data->dt_data->project, "RUBENS", 6) == 0)
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5707_RUBENS;
		else 
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5707;
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5708) {
		rmi4_data->firmware_name = FW_IMAGE_NAME_S5708;
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5006) {
		rmi4_data->firmware_name = FW_IMAGE_NAME_S5006;		
	} else if (rmi4_data->ic_version == SYNAPTICS_PRODUCT_ID_S5710) {
			rmi4_data->firmware_name = FW_IMAGE_NAME_S5710; 	
	} else {
		rmi4_data->firmware_name = FW_IMAGE_NAME_NONE;
	}

}

#ifdef TOUCHKEY_ENABLE
int synaptics_tkey_led_vdd_on(struct synaptics_rmi4_data *rmi4_data, bool onoff)
{
	struct device *dev = &rmi4_data->i2c_client->dev;
	int ret = -1;

	static struct regulator *reg_l17;

	if (!reg_l17) {
		reg_l17 = regulator_get(NULL, "8941_l17");
		if (IS_ERR(reg_l17)) {
			dev_err(dev, "could not get 8941_l17, rc = %ld=n",
				PTR_ERR(reg_l17));
			return ret;
		}
		ret = regulator_set_voltage(reg_l17, 3100000, 3100000);
		if (ret) {
			dev_err(dev, "%s: unable to set ldo17 voltage to 3.1V\n",
				__func__);
			return ret;
		}
	}

	if (onoff) {
		if (!regulator_is_enabled(reg_l17)) {
			ret = regulator_enable(reg_l17);
			if (ret) {
				pr_err("enable l17 failed, rc=%d\n", ret);
				return ret;
			}
			dev_info(dev, "keyled 3.1V on is finished.\n");
		} else
			dev_info(dev, "keyled 3.1V is already on.\n");
	} else {
		if (regulator_is_enabled(reg_l17)) {
			ret = regulator_disable(reg_l17);
			if (ret) {
				pr_err("disable l17 failed, rc=%d\n", ret);
				return ret;
			}
			dev_info(dev, "keyled 3.1V off is finished.\n");
		} else
			dev_info(dev, "keyled 3.1V is already off.\n");
	}

	return 0;
}

#ifdef CONFIG_LEDS_CLASS
static void msm_tkey_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(led_cdev, struct synaptics_rmi4_data, leds);

	if (value)
		rmi4_data->touchkey_led = true;
	else
		rmi4_data->touchkey_led = false;

	synaptics_tkey_led_vdd_on(rmi4_data, rmi4_data->touchkey_led);
}
#endif
#endif

#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
extern int get_lcd_attached(void);
#endif
/**
 * synaptics_rmi4_probe()
 *
 * Called by the kernel when an association with an I2C device of the
 * same name is made (after doing i2c_add_driver).
 *
 * This funtion allocates and initializes the resources for the driver
 * as an input driver, turns on the power to the sensor, queries the
 * sensor for its supported Functions and characteristics, registers
 * the driver to the input subsystem, sets up the interrupt, handles
 * the registration of the early_suspend and late_resume functions,
 * and creates a work queue for detection of other expansion Function
 * modules.
 */
static int __devinit synaptics_rmi4_probe(struct i2c_client *client,
		const struct i2c_device_id *dev_id)
{
	int retval, i;
	unsigned char attr_count;
	int attr_count_num;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_device_info *rmi;
	struct synaptics_rmi4_device_tree_data *dt_data;

	dev_err(&client->dev, "%s start.\n", __func__);
	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev,
				"%s: SMBus byte data not supported\n",
				__func__);
		return -EIO;
	}
#if defined(CONFIG_FB_MSM8x26_MDSS_CHECK_LCD_CONNECTION)
        if (get_lcd_attached() == 0) {
                dev_err(&client->dev, "%s : get_lcd_attached()=0 \n", __func__);
                return -EIO;
        }
#endif
	if (client->dev.of_node) {
		dt_data = devm_kzalloc(&client->dev,
				sizeof(struct synaptics_rmi4_device_tree_data),
				GFP_KERNEL);
		if (!dt_data) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
		retval = synaptics_parse_dt(&client->dev, dt_data);
		if (retval)
			return retval;
	} else	{
		dt_data = client->dev.platform_data;
		printk(KERN_ERR "TSP failed to align dtsi %s", __func__);
	}

	if (!dt_data) {
		dev_err(&client->dev,
				"%s: device tree data is not found\n",
				__func__);
		return -EINVAL;
	}

	rmi4_data = kzalloc(sizeof(*rmi4_data), GFP_KERNEL);
	if (!rmi4_data) {
		dev_err(&client->dev,
				"%s: Failed to alloc mem for rmi4_data\n",
				__func__);
		return -ENOMEM;
	}

	rmi = &(rmi4_data->rmi4_mod_info);

	rmi4_data->dt_data = dt_data;
	synaptics_request_gpio(rmi4_data);

	client->irq = gpio_to_irq(rmi4_data->dt_data->irq_gpio);
	rmi4_data->i2c_client = client;
	rmi4_data->current_page = MASK_8BIT;

	rmi4_data->touch_stopped = false;
	rmi4_data->sensor_sleep = false;
	rmi4_data->irq_enabled = false;
	rmi4_data->fingers_on_2d = false;
	rmi4_data->hand_edge_down = false;

	rmi4_data->i2c_read = synaptics_rmi4_i2c_read;
	rmi4_data->i2c_write = synaptics_rmi4_i2c_write;
	rmi4_data->irq_enable = synaptics_rmi4_irq_enable;
	rmi4_data->reset_device = synaptics_rmi4_reset_device;
	rmi4_data->stop_device = synaptics_rmi4_stop_device;
	rmi4_data->start_device = synaptics_rmi4_start_device;
	rmi4_data->irq = client->irq;

#ifdef READ_LCD_ID
	rmi4_data->lcd_id = synaptics_lcd_id;
#else
	rmi4_data->lcd_id = 0x00;
#endif

	mutex_init(&(rmi4_data->rmi4_io_ctrl_mutex));
	mutex_init(&(rmi4_data->rmi4_reset_mutex));
	mutex_init(&(rmi4_data->rmi4_reflash_mutex));
	mutex_init(&(rmi4_data->rmi4_device_mutex));

#ifdef USE_HOVER_REZERO
	INIT_DELAYED_WORK(&rmi4_data->rezero_work, synaptics_rmi4_rezero_work);
#endif
	i2c_set_clientdata(client, rmi4_data);

	rmi4_data->supplies = kzalloc(
		sizeof(struct regulator_bulk_data) * rmi4_data->dt_data->num_of_supply, GFP_KERNEL);
	if (!rmi4_data->supplies) {
		dev_err(&client->dev,
			"%s: Failed to alloc mem for supplies\n", __func__);
		retval = -ENOMEM;

		goto err_mem_regulator;
	}
	for (i = 0; i < rmi4_data->dt_data->num_of_supply; i++)
		rmi4_data->supplies[i].supply = rmi4_data->dt_data->name_of_supply[i];

	retval = regulator_bulk_get(&client->dev, rmi4_data->dt_data->num_of_supply,
				 rmi4_data->supplies);
	if (retval)
		goto err_get_regulator;
#ifdef COMMON_INPUT_BOOSTER
	rmi4_data->tsp_booster = kzalloc(sizeof(struct input_booster), GFP_KERNEL);
	if (!rmi4_data->tsp_booster) {
		dev_err(&client->dev,
			"%s: Failed to alloc mem for tsp_booster\n", __func__);
		goto err_get_tsp_booster;
	} else {
		input_booster_init_dvfs(rmi4_data->tsp_booster, INPUT_BOOSTER_ID_TSP);
	}
#endif

err_tsp_reboot:
	synaptics_power_ctrl(rmi4_data, true);
	msleep(SYNAPTICS_POWER_MARGIN_TIME);

#ifdef TSP_BOOSTER
	synaptics_init_dvfs(rmi4_data);
#endif
#ifdef TKEY_BOOSTER
	synaptics_tkey_init_dvfs(rmi4_data);
#endif

	retval = synaptics_rmi4_set_input_device(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to set up input device\n",
				__func__);

		if ((retval == TSP_NEEDTO_REBOOT) && (rmi4_data->rebootcount < MAX_TSP_REBOOT)) {
			synaptics_power_ctrl(rmi4_data, false);
			msleep(SYNAPTICS_POWER_MARGIN_TIME);
			rmi4_data->rebootcount++;
			dev_err(&client->dev,
					"%s: reboot sequence by i2c fail\n",
					__func__);
			goto err_tsp_reboot;
		} else {
			goto err_set_input_device;
		}
	}

	retval = synaptics_rmi4_init_exp_fn(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to register rmidev module\n",
				__func__);
		goto err_init_exp_fn;
	}

	retval = synaptics_rmi4_irq_enable(rmi4_data, true);
	if (retval < 0) {
		dev_err(&client->dev,
				"%s: Failed to enable attention interrupt\n",
				__func__);
		goto err_enable_irq;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&client->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			goto err_sysfs;
		}
	}

	synaptics_get_firmware_name(rmi4_data);
	retval = synaptics_rmi4_fw_update_on_probe(rmi4_data);
	if (retval < 0) {
		dev_err(&client->dev, "%s: Failed to firmware update\n",
				__func__);
		goto err_fw_update;
	}
#if defined(CONFIG_LEDS_CLASS) && defined(TOUCHKEY_ENABLE)
	rmi4_data->leds.name = TOUCHKEY_BACKLIGHT;
	rmi4_data->leds.brightness = LED_FULL;
	rmi4_data->leds.max_brightness = LED_FULL;
	rmi4_data->leds.brightness_set = msm_tkey_led_set;

	retval = led_classdev_register(&client->dev, &rmi4_data->leds);
	if (retval) {
		dev_err(&client->dev,
			"Failed to register led(%d)\n", retval);
		goto err_led_reg;
	}
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	rmi4_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 2;
	rmi4_data->early_suspend.suspend = synaptics_rmi4_early_suspend;
	rmi4_data->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&rmi4_data->early_suspend);
#endif

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
	synaptics_rmi_select_ta_mode(rmi4_data);
	if (rmi4_data->ta_con_mode) {
		rmi4_data->register_cb = synaptics_tsp_register_callback;

		rmi4_data->callbacks.inform_charger = synaptics_ta_cb;
		if (rmi4_data->register_cb) {
			dev_err(&client->dev, "Register TA Callback\n");
			rmi4_data->register_cb(&rmi4_data->callbacks);
		}
	}
#endif

	/* for blocking to be excuted open function until probing */
	rmi4_data->tsp_probe = true;
#if defined(CONFIG_SEC_HESTIA_PROJECT)
        retval = synaptics_rmi4_reset_device(rmi4_data);
        if (retval < 0) {
                dev_err(&client->dev,
                                "%s: Failed to issue reset command, error = %d\n",
                                __func__, retval);
                return retval;
        }
	msleep(SYNAPTICS_HW_RESET_TIME);
#endif
#ifdef SIDE_TOUCH
	/* default deepsleep mode */
	rmi4_data->use_deepsleep = DEFAULT_DISABLE;
#endif

#ifdef TSP_TURNOFF_AFTER_PROBE
/* turn off touch IC, will be turned by InputRedaer */
	synaptics_rmi4_stop_device(rmi4_data);
#endif
	return retval;

#if defined(CONFIG_LEDS_CLASS) && defined(TOUCHKEY_ENABLE)
err_led_reg:
#endif
err_fw_update:
err_sysfs:
	attr_count_num = (int)attr_count;
	for (attr_count_num--; attr_count_num >= 0; attr_count_num--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count_num].attr);
	}

	synaptics_rmi4_irq_enable(rmi4_data, false);
	dev_info(&rmi4_data->i2c_client->dev,
		"%s: driver unloading..\n", __func__);

err_enable_irq:
	synaptics_rmi4_remove_exp_fn(rmi4_data);

err_init_exp_fn:
	synaptics_rmi4_release_support_fn(rmi4_data);
#ifdef USE_OPEN_CLOSE
	rmi4_data->input_dev->open = NULL;
	rmi4_data->input_dev->close = NULL;
#endif
	input_unregister_device(rmi4_data->input_dev);
	input_free_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;
err_set_input_device:
	synaptics_power_ctrl(rmi4_data, false);
#ifdef COMMON_INPUT_BOOSTER
	kfree(rmi4_data->tsp_booster);
err_get_tsp_booster:
#endif
err_get_regulator:
	kfree(rmi4_data->supplies);
err_mem_regulator:
	mutex_destroy(&(rmi4_data->rmi4_io_ctrl_mutex));
	mutex_destroy(&(rmi4_data->rmi4_reset_mutex));
	mutex_destroy(&(rmi4_data->rmi4_reflash_mutex));
	mutex_destroy(&(rmi4_data->rmi4_device_mutex));

	kfree(rmi4_data);

	printk(KERN_ERR "%s: driver unload done..\n", __func__);
	return retval;
}

#ifdef USE_SHUTDOWN_CB
static void synaptics_rmi4_shutdown(struct i2c_client *client)
{
	struct synaptics_rmi4_data *rmi4_data = i2c_get_clientdata(client);

	dev_info(&rmi4_data->i2c_client->dev, "%s() is called\n", __func__);

	synaptics_rmi4_stop_device(rmi4_data);
}
#endif

/**
 * synaptics_rmi4_remove()
 *
 * Called by the kernel when the association with an I2C device of the
 * same name is broken (when the driver is unloaded).
 *
 * This funtion terminates the work queue, stops sensor data acquisition,
 * frees the interrupt, unregisters the driver from the input subsystem,
 * turns off the power to the sensor, and frees other allocated resources.
 */
static int __devexit synaptics_rmi4_remove(struct i2c_client *client)
{
	unsigned char attr_count;
	struct synaptics_rmi4_data *rmi4_data = i2c_get_clientdata(client);
	struct synaptics_rmi4_device_info *rmi;

	rmi = &(rmi4_data->rmi4_mod_info);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&rmi4_data->early_suspend);
#endif
#if defined(CONFIG_LEDS_CLASS) && defined(TOUCHKEY_ENABLE)
	led_classdev_unregister(&rmi4_data->leds);
#endif
	synaptics_rmi4_irq_enable(rmi4_data, false);

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	synaptics_rmi4_remove_exp_fn(rmi4_data);

	synaptics_power_ctrl(rmi4_data, false);
	rmi4_data->touch_stopped = true;

	synaptics_rmi4_release_support_fn(rmi4_data);

#ifdef USE_OPEN_CLOSE
	rmi4_data->input_dev->open = NULL;
	rmi4_data->input_dev->close = NULL;
#endif

	input_unregister_device(rmi4_data->input_dev);
	input_free_device(rmi4_data->input_dev);
	rmi4_data->input_dev = NULL;

#ifdef COMMON_INPUT_BOOSTER
	kfree(rmi4_data->tsp_booster);
#endif

	kfree(rmi4_data->supplies);

	mutex_destroy(&(rmi4_data->rmi4_io_ctrl_mutex));
	mutex_destroy(&(rmi4_data->rmi4_reset_mutex));
	mutex_destroy(&(rmi4_data->rmi4_reflash_mutex));
	mutex_destroy(&(rmi4_data->rmi4_device_mutex));

	kfree(rmi4_data);

	return 0;
}

#ifdef USE_SENSOR_SLEEP
/**
 * synaptics_rmi4_sensor_sleep()
 *
 * Called by synaptics_rmi4_early_suspend() and synaptics_rmi4_suspend().
 *
 * This function stops finger data acquisition and puts the sensor to sleep.
 */
static void synaptics_rmi4_sensor_sleep(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;
#ifdef SIDE_TOUCH
	unsigned char sidekey_only_active;
#endif
	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

#ifdef SIDE_TOUCH
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->general_control2_addr,
			&sidekey_only_active,
			sizeof(sidekey_only_active));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to read f51_general_control2\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	sidekey_only_active |= SIDE_TOUCH_ONLY_ACTIVE;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->general_control2_addr,
			&sidekey_only_active,
			sizeof(sidekey_only_active));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to write f51_general_control2\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	rmi4_data->sidekey_only_enable = true;

#endif

	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to enter sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	device_ctrl |= SENSOR_SLEEP;

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to enter sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	} else {
		rmi4_data->sensor_sleep = true;
	}

	msleep(20);
	synaptics_rmi4_free_fingers(rmi4_data);
#ifdef DISABLE_IRQ_WHEN_ENTER_DEEPSLEEP
	synaptics_rmi4_irq_enable(rmi4_data, false);
#endif
	return;
}

/**
 * synaptics_rmi4_sensor_wake()
 *
 * Called by synaptics_rmi4_resume() and synaptics_rmi4_late_resume().
 *
 * This function wakes the sensor from sleep.
 */
static void synaptics_rmi4_sensor_wake(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char device_ctrl;
#ifdef SIDE_TOUCH
	unsigned char sidekey_only_active;
#endif
	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

#ifdef DISABLE_IRQ_WHEN_ENTER_DEEPSLEEP
	synaptics_rmi4_irq_enable(rmi4_data, true);
#endif
#ifdef SIDE_TOUCH
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f51_handle->general_control2_addr,
			&sidekey_only_active,
			sizeof(sidekey_only_active));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to read f51_general_control2\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	sidekey_only_active &= ~(SIDE_TOUCH_ONLY_ACTIVE);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f51_handle->general_control2_addr,
			&sidekey_only_active,
			sizeof(sidekey_only_active));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to write f51_general_control2\n",
				__func__);
		rmi4_data->sensor_sleep = false;
		return;
	}

	rmi4_data->sidekey_only_enable = false;

#endif
	retval = synaptics_rmi4_i2c_read(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to wake from sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = true;
		return;
	}

	device_ctrl &= ~(SENSOR_SLEEP);

	retval = synaptics_rmi4_i2c_write(rmi4_data,
			rmi4_data->f01_ctrl_base_addr,
			&device_ctrl,
			sizeof(device_ctrl));
	if (retval < 0) {
		dev_err(&(rmi4_data->i2c_client->dev),
				"%s: Failed to wake from sleep mode\n",
				__func__);
		rmi4_data->sensor_sleep = true;
		return;
	} else {
		rmi4_data->sensor_sleep = false;
	}

	return;
}
#endif

static int synaptics_rmi4_stop_device(struct synaptics_rmi4_data *rmi4_data)
{
	mutex_lock(&rmi4_data->rmi4_device_mutex);

	if (rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s already power off\n",
				__func__);
		goto out;
	}

	disable_irq(rmi4_data->i2c_client->irq);
	synaptics_rmi4_free_fingers(rmi4_data);
	rmi4_data->touch_stopped = true;
	synaptics_power_ctrl(rmi4_data, false);

	dev_dbg(&rmi4_data->i2c_client->dev, "%s\n", __func__);

out:
	mutex_unlock(&rmi4_data->rmi4_device_mutex);
	return 0;
}

static int synaptics_rmi4_start_device(struct synaptics_rmi4_data *rmi4_data)
{
	int retval = 0;
	mutex_lock(&rmi4_data->rmi4_device_mutex);

	if (!rmi4_data->touch_stopped) {
		dev_err(&rmi4_data->i2c_client->dev, "%s already power on\n",
				__func__);
		goto out;
	}

	synaptics_power_ctrl(rmi4_data, true);
	rmi4_data->current_page = MASK_8BIT;
	rmi4_data->touch_stopped = false;

	msleep(SYNAPTICS_HW_RESET_TIME);

#if defined(CONFIG_SEC_FACTORY)
	retval = synaptics_rmi4_query_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: Failed to query device\n",
			__func__);
	}
#else
	retval = synaptics_rmi4_reinit_device(rmi4_data);
	if (retval < 0) {
		dev_err(&rmi4_data->i2c_client->dev,
			"%s: Failed to reinit device\n",
			__func__);
	}
#endif

#ifdef SYNAPTICS_RMI_INFORM_CHARGER
	if (rmi4_data->ta_con_mode)
		synaptics_charger_conn(rmi4_data, rmi4_data->ta_status);
#endif

	enable_irq(rmi4_data->i2c_client->irq);

	dev_dbg(&rmi4_data->i2c_client->dev, "%s\n", __func__);

out:
	mutex_unlock(&rmi4_data->rmi4_device_mutex);
	return retval;
}

#ifdef USE_OPEN_CLOSE
static int synaptics_rmi4_input_open(struct input_dev *dev)
{
	struct synaptics_rmi4_data *rmi4_data = input_get_drvdata(dev);
	int retval;

	dev_info(&rmi4_data->i2c_client->dev, "%s %s\n", __func__, rmi4_data->use_deepsleep ? "wakeup" : "");

	gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->scl_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->sda_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	if (rmi4_data->use_deepsleep) {
		synaptics_rmi4_sensor_wake(rmi4_data);
	} else {
		retval = synaptics_rmi4_start_device(rmi4_data);
		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to start device\n", __func__);
	}

	return 0;
}

static void synaptics_rmi4_input_close(struct input_dev *dev)
{
	struct synaptics_rmi4_data *rmi4_data = input_get_drvdata(dev);

	dev_info(&rmi4_data->i2c_client->dev, "%s %s\n", __func__, rmi4_data->use_deepsleep ? "deepsleep" : "");

	if (rmi4_data->use_deepsleep) {
		synaptics_rmi4_sensor_sleep(rmi4_data);
		gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->scl_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->sda_gpio, 3, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	} else {
		synaptics_rmi4_stop_device(rmi4_data);
		gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->scl_gpio, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(rmi4_data->dt_data->sda_gpio, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	}
}
#endif

#ifdef CONFIG_PM
#ifdef CONFIG_HAS_EARLYSUSPEND
#define synaptics_rmi4_suspend NULL
#define synaptics_rmi4_resume NULL

/**
 * synaptics_rmi4_early_suspend()
 *
 * Called by the kernel during the early suspend phase when the system
 * enters suspend.
 *
 * This function calls synaptics_rmi4_sensor_sleep() to stop finger
 * data acquisition and put the sensor to sleep.
 */
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4_data *rmi4_data =
		container_of(h, struct synaptics_rmi4_data,
				early_suspend);

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

	if (rmi4_data->stay_awake) {
		rmi4_data->staying_awake = true;
		dev_info(&rmi4_data->i2c_client->dev, "%s : return due to staying_awake\n",
				__func__);
		return;
	} else {
		rmi4_data->staying_awake = false;
	}

	synaptics_rmi4_stop_device(rmi4_data);

	return;
}

/**
 * synaptics_rmi4_late_resume()
 *
 * Called by the kernel during the late resume phase when the system
 * wakes up from suspend.
 *
 * This function goes through the sensor wake process if the system wakes
 * up from early suspend (without going into suspend).
 */
static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	int retval = 0;
	struct synaptics_rmi4_data *rmi4_data =
		container_of(h, struct synaptics_rmi4_data,
				early_suspend);

	dev_info(&rmi4_data->i2c_client->dev, "%s\n", __func__);

	if (rmi4_data->staying_awake) {
		dev_info(&rmi4_data->i2c_client->dev, "%s : return due to staying_awake\n",
				__func__);
		return;
	}

	retval = synaptics_rmi4_start_device(rmi4_data);
	if (retval < 0)
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to start device\n", __func__);

	return;
}
#else

/**
 * synaptics_rmi4_suspend()
 *
 * Called by the kernel during the suspend phase when the system
 * enters suspend.
 *
 * This function stops finger data acquisition and puts the sensor to
 * sleep (if not already done so during the early suspend phase),
 * disables the interrupt, and turns off the power to the sensor.
 */
static int synaptics_rmi4_suspend(struct device *dev)
{
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	dev_dbg(&rmi4_data->i2c_client->dev, "%s\n", __func__);

	if (rmi4_data->staying_awake) {
		dev_info(&rmi4_data->i2c_client->dev, "%s : return due to staying_awake\n",
				__func__);
		return 0;
	}

	mutex_lock(&rmi4_data->input_dev->mutex);

	if (rmi4_data->input_dev->users)
		synaptics_rmi4_stop_device(rmi4_data);

	mutex_unlock(&rmi4_data->input_dev->mutex);

	return 0;
}

/**
 * synaptics_rmi4_resume()
 *
 * Called by the kernel during the resume phase when the system
 * wakes up from suspend.
 *
 * This function turns on the power to the sensor, wakes the sensor
 * from sleep, enables the interrupt, and starts finger data
 * acquisition.
 */
static int synaptics_rmi4_resume(struct device *dev)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = dev_get_drvdata(dev);

	dev_dbg(&rmi4_data->i2c_client->dev, "%s\n", __func__);

	mutex_lock(&rmi4_data->input_dev->mutex);

	if (rmi4_data->input_dev->users) {
		retval = synaptics_rmi4_start_device(rmi4_data);
		if (retval < 0)
			dev_err(&rmi4_data->i2c_client->dev,
					"%s: Failed to start device\n", __func__);
	}

	mutex_unlock(&rmi4_data->input_dev->mutex);

	return 0;
}
#endif

static const struct dev_pm_ops synaptics_rmi4_dev_pm_ops = {
	.suspend = synaptics_rmi4_suspend,
	.resume  = synaptics_rmi4_resume,
};
#endif

static const struct i2c_device_id synaptics_rmi4_id_table[] = {
	{DRIVER_NAME, 0},
	{},
};
MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_id_table);


#ifdef CONFIG_OF
static struct of_device_id synaptics_match_table[] = {
	{ .compatible = "synaptics,rmi4-ts",},
	{ },
};
#else
#define synaptics_match_table	NULL
#endif

static struct i2c_driver synaptics_rmi4_driver = {
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = synaptics_match_table,
#ifdef CONFIG_PM
		.pm = &synaptics_rmi4_dev_pm_ops,
#endif
	},
	.probe = synaptics_rmi4_probe,
	.remove = __devexit_p(synaptics_rmi4_remove),
#ifdef USE_SHUTDOWN_CB
	.shutdown = synaptics_rmi4_shutdown,
#endif
	.id_table = synaptics_rmi4_id_table,
};

/**
 * synaptics_rmi4_init()
 *
 * Called by the kernel during do_initcalls (if built-in)
 * or when the driver is loaded (if a module).
 *
 * This function registers the driver to the I2C subsystem.
 *
 */
static int __init synaptics_rmi4_init(void)
{
#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif
	return i2c_add_driver(&synaptics_rmi4_driver);
}

/**
 * synaptics_rmi4_exit()
 *
 * Called by the kernel when the driver is unloaded.
 *
 * This funtion unregisters the driver from the I2C subsystem.
 *
 */
static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_AUTHOR("Synaptics, Inc.");
MODULE_DESCRIPTION("Synaptics RMI4 I2C Touch Driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SYNAPTICS_RMI4_DRIVER_VERSION);

