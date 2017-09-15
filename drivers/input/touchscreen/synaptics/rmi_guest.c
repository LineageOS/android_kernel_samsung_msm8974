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
#include <asm/siginfo.h>
//#include <mach/cpufreq.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/signal.h>
#include <linux/input/synaptics_dsx.h>
#include "synaptics_i2c_rmi.h"

#include "Multiverse/GMvSystem.h"

#define GEUST_PAGES_TO_SERVICE (0x60)
#define HARDCODE_PDT

static ssize_t rmi_guest_sysfs_query_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t rmi_guest_sysfs_control_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t rmi_guest_sysfs_data_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf);

static ssize_t rmi_guest_sysfs_command_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf);

struct rmi_guest_handle {
	unsigned char intr_mask;
	unsigned char intr_reg_num;
	unsigned short query_base_addr;
	unsigned short control_base_addr;
	unsigned short data_base_addr;
	unsigned short command_base_addr;
	struct synaptics_rmi4_data *rmi4_data;
	struct synaptics_rmi4_exp_fn_ptr *fn_ptr;
	struct kobject *sysfs_dir;
};

static struct device_attribute attrs[] = {
	__ATTR(query_base_addr, S_IRUGO,
			rmi_guest_sysfs_query_base_addr_show,
			synaptics_rmi4_store_error),
	__ATTR(control_base_addr, S_IRUGO,
			rmi_guest_sysfs_control_base_addr_show,
			synaptics_rmi4_store_error),
	__ATTR(data_base_addr, S_IRUGO,
			rmi_guest_sysfs_data_base_addr_show,
			synaptics_rmi4_store_error),
	__ATTR(command_base_addr, S_IRUGO,
			rmi_guest_sysfs_command_base_addr_show,
			synaptics_rmi4_store_error),
};

static struct rmi_guest_handle *rmiguest;

static ssize_t rmi_guest_sysfs_query_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%04x\n",
			rmiguest->query_base_addr);
}

static ssize_t rmi_guest_sysfs_control_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%04x\n",
			rmiguest->control_base_addr);
}

static ssize_t rmi_guest_sysfs_data_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%04x\n",
			rmiguest->data_base_addr);
}

static ssize_t rmi_guest_sysfs_command_base_addr_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "0x%04x\n",
			rmiguest->command_base_addr);
}

static int rmi_guest_scan_pdt(void)
{
	int retval;
	unsigned short ii;
	unsigned short addr;
	unsigned char page;
	unsigned char intr_count = 0;
	unsigned char intr_offset;
	bool fguest_found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	struct synaptics_rmi4_data *rmi4_data = rmiguest->rmi4_data;

	for (page = 0; page <= GEUST_PAGES_TO_SERVICE; page++) {
		for (ii = PDT_START; ii > PDT_END; ii -= PDT_ENTRY_SIZE) {
			ii |= (page << 8);

			retval = rmiguest->fn_ptr->read(rmiguest->rmi4_data,
					ii,
					(unsigned char *)&rmi_fd,
					sizeof(rmi_fd));
			if (retval < 0)
				return retval;

			ii &= ~(MASK_8BIT << 8);

			if (!rmi_fd.fn_number)
				break;

			if (rmi_fd.fn_number == SYNAPTICS_RMI4_F60) {
				fguest_found = true;
				goto fguest_found;
			}

			intr_count += (rmi_fd.intr_src_count & MASK_3BIT);
		}
	}

#ifndef HARDCODE_PDT
	if (!fguest_found) {
		dev_err(&rmiguest->rmi4_data->i2c_client->dev,
				"%s: Failed to find F60\n",
				__func__);
		return -EINVAL;
	}
#endif

fguest_found:
	rmiguest->query_base_addr = rmi_fd.query_base_addr | (page << 8);
	rmiguest->control_base_addr = rmi_fd.ctrl_base_addr | (page << 8);
	rmiguest->data_base_addr = rmi_fd.data_base_addr | (page << 8);
	rmiguest->command_base_addr = rmi_fd.cmd_base_addr | (page << 8);

#ifdef HARDCODE_PDT
	rmiguest->query_base_addr = 0x0000;
	rmiguest->control_base_addr = 0x0000;
	rmiguest->data_base_addr = 0x6000;
	rmiguest->command_base_addr = 0x0000;
	rmi_fd.intr_src_count = 1;
#endif

	rmiguest->intr_reg_num = (intr_count + 7) / 8;
	if (rmiguest->intr_reg_num != 0)
		rmiguest->intr_reg_num -= 1;

	rmiguest->intr_mask = 0;
	intr_offset = intr_count % 8;
	for (ii = intr_offset;
			ii < ((rmi_fd.intr_src_count & MASK_3BIT) +
			intr_offset);
			ii++) {
		rmiguest->intr_mask |= 1 << ii;
	}

//	pr_info("GT interrupt mask = 0x%02x\n", rmiguest->intr_mask);

	rmi4_data->intr_mask[0] |= rmiguest->intr_mask;

	addr = rmi4_data->f01_ctrl_base_addr + 1;

	retval = rmiguest->fn_ptr->write(rmi4_data,
			addr,
			&(rmi4_data->intr_mask[0]),
			sizeof(rmi4_data->intr_mask[0]));
	if (retval < 0)
		return retval;

	return 0;
}

static void rmi_guest_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	if (!rmiguest)
		return;

	if (rmiguest->intr_mask & intr_mask)
		GMvBraneIsr();

	return;
}

static int rmi_guest_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	unsigned char attr_count;
	int attr_count_num;

	rmiguest = kzalloc(sizeof(*rmiguest), GFP_KERNEL);
	if (!rmiguest) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for rmiguest\n",
				__func__);
		retval = -ENOMEM;
		goto err_rmi_guest;
	}

	rmiguest->fn_ptr =  kzalloc(sizeof(*(rmiguest->fn_ptr)), GFP_KERNEL);
	if (!rmiguest->fn_ptr) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to alloc mem for fn_ptr\n",
				__func__);
		retval = -ENOMEM;
		goto err_fn_ptr;
	}

	rmiguest->fn_ptr->read = rmi4_data->i2c_read;
	rmiguest->fn_ptr->write = rmi4_data->i2c_write;
	rmiguest->fn_ptr->enable = rmi4_data->irq_enable;
	rmiguest->rmi4_data = rmi4_data;

	retval = rmi_guest_scan_pdt();
	if (retval < 0) {
		retval = 0;
		goto err_scan_pdt;
	}

	rmiguest->sysfs_dir = kobject_create_and_add("guest",
			&rmi4_data->input_dev->dev.kobj);
	if (!rmiguest->sysfs_dir) {
		dev_err(&rmi4_data->i2c_client->dev,
				"%s: Failed to create sysfs directory\n",
				__func__);
		retval = -ENODEV;
		goto err_sysfs_dir;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(rmiguest->sysfs_dir,
				&attrs[attr_count].attr);
		if (retval < 0) {
			dev_err(&rmi4_data->input_dev->dev,
					"%s: Failed to create sysfs attributes\n",
					__func__);
			retval = -ENODEV;
			goto err_sysfs_attrs;
		}
	}

	return 0;

err_sysfs_attrs:
	attr_count_num = (int)attr_count;
	for (attr_count_num--; attr_count_num >= 0; attr_count_num--)
		sysfs_remove_file(rmiguest->sysfs_dir, &attrs[attr_count_num].attr);

	kobject_put(rmiguest->sysfs_dir);

err_sysfs_dir:
err_scan_pdt:
	kfree(rmiguest->fn_ptr);

err_fn_ptr:
	kfree(rmiguest);
	rmiguest = NULL;

err_rmi_guest:
	return retval;
}

static void rmi_guest_remove(struct synaptics_rmi4_data *rmi4_data)
{
	unsigned char attr_count;

	if (!rmiguest)
		goto exit;

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++)
		sysfs_remove_file(rmiguest->sysfs_dir, &attrs[attr_count].attr);

	kobject_put(rmiguest->sysfs_dir);

	kfree(rmiguest->fn_ptr);
	kfree(rmiguest);
	rmiguest = NULL;

exit:
	return;
}

int rmi_guest_module_register(void)
{
	int retval;

	retval = synaptics_rmi4_new_function(RMI_GUEST,
			rmi_guest_init,
			rmi_guest_remove,
			rmi_guest_attn);

	return retval;
}

void GMvGtReadI2cRegister( uint16 u16Addr, void *pvData, sint32 s32Size )
{
	if( rmiguest )
	{
		if( rmiguest->fn_ptr && rmiguest->rmi4_data )
		{
			if( rmiguest->fn_ptr->read )
				rmiguest->fn_ptr->read( rmiguest->rmi4_data, u16Addr, pvData, s32Size );
		}
	}
}

void GMvGtWriteI2cRegister( uint16 u16Addr, void *pvData, sint32 s32Size )
{
	if( rmiguest )
	{
		if( rmiguest->fn_ptr && rmiguest->rmi4_data )
		{
			if( rmiguest->fn_ptr->write )
				rmiguest->fn_ptr->write( rmiguest->rmi4_data, u16Addr, pvData, s32Size );
		}
	}
}
