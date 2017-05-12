/* Copyright (c) 2012-2017, The Linux Foundation. All rights reserved.
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
 */

#include "msm_vidc_debug.h"
#include "vidc_hfi_api.h"

#define MAX_DBG_BUF_SIZE 4096
int msm_vidc_debug = VIDC_ERR | VIDC_WARN;
int msm_vidc_debug_out = VIDC_OUT_PRINTK;
int msm_fw_debug = 0x18;
int msm_fw_debug_mode = 0x1;
int msm_fw_low_power_mode = 0x1;
int msm_vidc_hw_rsp_timeout = 1000;
u32 msm_vidc_firmware_unload_delay = 15000;
int msm_vp8_low_tier = 0x0;/* 0x1; *//* changed to support 3840x2160 VP8 */
int msm_vidc_vpe_csc_601_to_709;

#define DYNAMIC_BUF_OWNER(__binfo) ({ \
	atomic_read(&__binfo->ref_count) == 2 ? "video driver" : "firmware";\
})

struct core_inst_pair {
	struct msm_vidc_core *core;
	struct msm_vidc_inst *inst;
};

static int core_info_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static u32 write_str(char *buffer,
		size_t size, const char *fmt, ...)
{
	va_list args;
	u32 len;

	va_start(args, fmt);
	len = vscnprintf(buffer, size, fmt, args);
	va_end(args);
	return len;
}

static ssize_t core_info_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct msm_vidc_core *core = file->private_data;
	struct hfi_device *hdev;
	struct hal_fw_info fw_info;
	char *dbuf, *cur, *end;
	int i = 0, rc = 0;
	ssize_t len = 0;

	if (!core || !core->device) {
		dprintk(VIDC_ERR, "Invalid params, core: %pK\n", core);
		return 0;
	}

	dbuf = kzalloc(MAX_DBG_BUF_SIZE, GFP_KERNEL);
	if (!dbuf) {
		dprintk(VIDC_ERR, "%s: Allocation failed!\n", __func__);
		return -ENOMEM;
	}
	cur = dbuf;
	end = cur + MAX_DBG_BUF_SIZE;
	hdev = core->device;

	cur += write_str(cur, end - cur, "===============================\n");
	cur += write_str(cur, end - cur, "CORE %d: %pK\n", core->id, core);
	cur += write_str(cur, end - cur, "===============================\n");
	cur += write_str(cur, end - cur, "Core state: %d\n", core->state);
	rc = call_hfi_op(hdev, get_fw_info, hdev->hfi_device_data, &fw_info);
        if (rc) {
                dprintk(VIDC_WARN, "Failed to read FW info\n");
                goto err_fw_info;
        }

	cur += write_str(cur, end - cur,
		"FW version : %s\n", &fw_info.version);
	cur += write_str(cur, end - cur,
		"base addr: 0x%x\n", fw_info.base_addr);
	cur += write_str(cur, end - cur,
		"register_base: 0x%x\n", fw_info.register_base);
	cur += write_str(cur, end - cur,
		"register_size: %u\n", fw_info.register_size);
	cur += write_str(cur, end - cur, "irq: %u\n", fw_info.irq);

	cur += write_str(cur, end - cur, "clock count: %d\n",
		call_hfi_op(hdev, get_info, hdev->hfi_device_data,
					DEV_CLOCK_COUNT));
	cur += write_str(cur, end - cur, "clock enabled: %u\n",
		call_hfi_op(hdev, get_info, hdev->hfi_device_data,
					DEV_CLOCK_ENABLED));
	cur += write_str(cur, end - cur, "power count: %d\n",
		call_hfi_op(hdev, get_info, hdev->hfi_device_data,
					DEV_PWR_COUNT));
	cur += write_str(cur, end - cur, "power enabled: %u\n",
		call_hfi_op(hdev, get_info, hdev->hfi_device_data,
					DEV_PWR_ENABLED));
err_fw_info:
	for (i = SYS_MSG_START; i < SYS_MSG_END; i++) {
		cur += write_str(cur, end - cur, "completions[%d]: %s\n", i,
			completion_done(&core->completions[SYS_MSG_INDEX(i)]) ?
			"pending" : "done");
	}
	len = simple_read_from_buffer(buf, count, ppos,
			dbuf, cur - dbuf);

	return len;
}

static const struct file_operations core_info_fops = {
	.open = core_info_open,
	.read = core_info_read,
};

static int trigger_ssr_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t trigger_ssr_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *ppos) {
	u32 ssr_trigger_val;
	int rc;
	struct msm_vidc_core *core = filp->private_data;
	rc = sscanf(buf, "%d", &ssr_trigger_val);
	if (rc < 0) {
		dprintk(VIDC_WARN, "returning error err %d\n", rc);
		rc = -EINVAL;
	} else {
		msm_vidc_trigger_ssr(core, ssr_trigger_val);
		rc = count;
	}
	return rc;
}

static const struct file_operations ssr_fops = {
	.open = trigger_ssr_open,
	.write = trigger_ssr_write,
};

struct dentry *msm_vidc_debugfs_init_core(struct msm_vidc_core *core,
		struct dentry *parent)
{
	struct dentry *dir = NULL;
	char debugfs_name[MAX_DEBUGFS_NAME];
	if (!core) {
		dprintk(VIDC_ERR, "Invalid params, core: %pK\n", core);
		goto failed_create_dir;
	}

	snprintf(debugfs_name, MAX_DEBUGFS_NAME, "core%d", core->id);
	dir = debugfs_create_dir(debugfs_name, parent);
	if (!dir) {
		dprintk(VIDC_ERR, "Failed to create debugfs for msm_vidc\n");
		goto failed_create_dir;
	}

	if (!debugfs_create_file("info", S_IRUGO, dir, core, &core_info_fops)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("debug_level", S_IRUGO | S_IWUSR,
			parent,	&msm_vidc_debug)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("fw_level", S_IRUGO | S_IWUSR,
			parent, &msm_fw_debug)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_file("trigger_ssr", S_IWUSR,
			dir, core, &ssr_fops)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("fw_debug_mode", S_IRUGO | S_IWUSR,
			parent, &msm_fw_debug_mode)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("fw_low_power_mode", S_IRUGO | S_IWUSR,
			parent, &msm_fw_low_power_mode)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("debug_output", S_IRUGO | S_IWUSR,
			parent, &msm_vidc_debug_out)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("hw_rsp_timeout", S_IRUGO | S_IWUSR,
			parent, &msm_vidc_hw_rsp_timeout)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("firmware_unload_delay", S_IRUGO | S_IWUSR,
			parent, &msm_vidc_firmware_unload_delay)) {
		dprintk(VIDC_ERR,
			"debugfs_create_file: firmware_unload_delay fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_u32("vp8_low_tier", S_IRUGO | S_IWUSR,
			parent, &msm_vp8_low_tier)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_bool("enable_vpe_csc_601_709", S_IRUGO | S_IWUSR,
			dir, &msm_vidc_vpe_csc_601_to_709)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
failed_create_dir:
	return dir;
}

static int inst_info_open(struct inode *inode, struct file *file)
{
	dprintk(VIDC_INFO, "Open inode ptr: %pK\n", inode->i_private);
	file->private_data = inode->i_private;
	return 0;
}

static int publish_unreleased_reference(struct msm_vidc_inst *inst,
		char **dbuf, char *end)
{
	char *cur = *dbuf;
	struct buffer_info *temp = NULL;
	struct buffer_info *dummy = NULL;
	struct list_head *list = NULL;

	if (!inst) {
		dprintk(VIDC_ERR, "%s: invalid param\n", __func__);
		return -EINVAL;
	}

	list = &inst->registered_bufs;
	mutex_lock(&inst->lock);
	if (inst->buffer_mode_set[CAPTURE_PORT] == HAL_BUFFER_MODE_DYNAMIC) {
		list_for_each_entry_safe(temp, dummy, list, list) {
			if (temp && temp->type ==
			V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE &&
			!temp->inactive && atomic_read(&temp->ref_count)) {
				cur += write_str(cur, end - cur,
				"\tpending buffer: 0x%x fd[0] = %d ref_count = %d held by: %s\n",
				temp->device_addr[0],
				temp->fd[0],
				atomic_read(&temp->ref_count),
				DYNAMIC_BUF_OWNER(temp));
			}
		}
	}
	mutex_unlock(&inst->lock);
	return 0;
}

static ssize_t inst_info_read(struct file *file, char __user *buf,
		size_t count, loff_t *ppos)
{
	struct msm_vidc_inst *inst = file->private_data;
	char *dbuf, *cur, *end;
	int i, j;
	ssize_t len = 0;

	if (!inst) {
		dprintk(VIDC_ERR, "Invalid params, core: %pK\n", inst);
		return 0;
	}

	dbuf = kzalloc(MAX_DBG_BUF_SIZE, GFP_KERNEL);
	if (!dbuf) {
		dprintk(VIDC_ERR, "%s: Allocation failed!\n", __func__);
		return -ENOMEM;
	}
	cur = dbuf;
	end = cur + MAX_DBG_BUF_SIZE;

	cur += write_str(cur, end - cur, "===============================\n");
	cur += write_str(cur, end - cur, "INSTANCE: 0x%pK (%s)\n", inst,
		inst->session_type == MSM_VIDC_ENCODER ? "Encoder" : "Decoder");
	cur += write_str(cur, end - cur, "===============================\n");
	cur += write_str(cur, end - cur, "core: 0x%pK\n", inst->core);
	cur += write_str(cur, end - cur, "height: %d\n", inst->prop.height[CAPTURE_PORT]);
	cur += write_str(cur, end - cur, "width: %d\n", inst->prop.width[CAPTURE_PORT]);
	cur += write_str(cur, end - cur, "fps: %d\n", inst->prop.fps);
	cur += write_str(cur, end - cur, "state: %d\n", inst->state);
	cur += write_str(cur, end - cur, "-----------Formats-------------\n");
	for (i = 0; i < MAX_PORT_NUM; i++) {
		cur += write_str(cur, end - cur, "capability: %s\n", i == OUTPUT_PORT ?
			"Output" : "Capture");
		cur += write_str(cur, end - cur, "name : %s\n", inst->fmts[i]->name);
		cur += write_str(cur, end - cur, "planes : %d\n", inst->fmts[i]->num_planes);
		cur += write_str(
		cur, end - cur, "type: %s\n", inst->fmts[i]->type == OUTPUT_PORT ?
		"Output" : "Capture");
		switch (inst->buffer_mode_set[i]) {
		case HAL_BUFFER_MODE_STATIC:
			cur += write_str(cur, end - cur, "buffer mode : %s\n", "static");
			break;
		case HAL_BUFFER_MODE_RING:
			cur += write_str(cur, end - cur, "buffer mode : %s\n", "ring");
			break;
		case HAL_BUFFER_MODE_DYNAMIC:
			cur += write_str(cur, end - cur, "buffer mode : %s\n", "dynamic");
			break;
		default:
			cur += write_str(cur, end - cur, "buffer mode : unsupported\n");
		}
		for (j = 0; j < inst->fmts[i]->num_planes; j++)
			cur += write_str(cur, end - cur, "size for plane %d: %u\n", j,
			inst->bufq[i].vb2_bufq.plane_sizes[j]);
	}
	cur += write_str(cur, end - cur, "-------------------------------\n");
	for (i = SESSION_MSG_START; i < SESSION_MSG_END; i++) {
		cur += write_str(cur, end - cur, "completions[%d]: %s\n", i,
		completion_done(&inst->completions[SESSION_MSG_INDEX(i)]) ?
		"pending" : "done");
	}
	cur += write_str(cur, end - cur, "ETB Count: %d\n", inst->count.etb);
	cur += write_str(cur, end - cur, "EBD Count: %d\n", inst->count.ebd);
	cur += write_str(cur, end - cur, "FTB Count: %d\n", inst->count.ftb);
	cur += write_str(cur, end - cur, "FBD Count: %d\n", inst->count.fbd);
	publish_unreleased_reference(inst, &dbuf, end);
	len = simple_read_from_buffer(buf, count, ppos,
		dbuf, cur - dbuf);
	return len;
}

static const struct file_operations inst_info_fops = {
	.open = inst_info_open,
	.read = inst_info_read,
};

struct dentry *msm_vidc_debugfs_init_inst(struct msm_vidc_inst *inst,
		struct dentry *parent)
{
	struct dentry *dir = NULL;
	char debugfs_name[MAX_DEBUGFS_NAME];
	if (!inst) {
		dprintk(VIDC_ERR, "Invalid params, inst: %pK\n", inst);
		goto failed_create_dir;
	}
	snprintf(debugfs_name, MAX_DEBUGFS_NAME, "inst_%pK", inst);
	dir = debugfs_create_dir(debugfs_name, parent);
	if (!dir) {
		dprintk(VIDC_ERR, "Failed to create debugfs for msm_vidc\n");
		goto failed_create_dir;
	}
	if (!debugfs_create_file("info", S_IRUGO, dir, inst, &inst_info_fops)) {
		dprintk(VIDC_ERR, "debugfs_create_file: fail\n");
		goto failed_create_dir;
	}
	inst->debug.pdata[FRAME_PROCESSING].sampling = true;
failed_create_dir:
	return dir;
}

void msm_vidc_debugfs_update(struct msm_vidc_inst *inst,
	enum msm_vidc_debugfs_event e)
{
	struct msm_vidc_debug *d = &inst->debug;
	char a[64] = "Frame processing";
	switch (e) {
	case MSM_VIDC_DEBUGFS_EVENT_ETB:
		inst->count.etb++;
		if (inst->count.ebd && inst->count.ftb > inst->count.fbd) {
			d->pdata[FRAME_PROCESSING].name[0] = '\0';
			tic(inst, FRAME_PROCESSING, a);
		}
	break;
	case MSM_VIDC_DEBUGFS_EVENT_EBD:
		inst->count.ebd++;
		if (inst->count.ebd && inst->count.ebd == inst->count.etb) {
			toc(inst, FRAME_PROCESSING);
			dprintk(VIDC_PROF, "EBD: FW needs input buffers\n");
		}
		if (inst->count.ftb == inst->count.fbd)
			dprintk(VIDC_PROF, "EBD: FW needs output buffers\n");
	break;
	case MSM_VIDC_DEBUGFS_EVENT_FTB: {
		inst->count.ftb++;
		if (inst->count.ebd && inst->count.etb > inst->count.ebd) {
			d->pdata[FRAME_PROCESSING].name[0] = '\0';
			tic(inst, FRAME_PROCESSING, a);
		}
	}
	break;
	case MSM_VIDC_DEBUGFS_EVENT_FBD:
		inst->debug.samples++;
		if (inst->count.ebd && inst->count.fbd == inst->count.ftb) {
			toc(inst, FRAME_PROCESSING);
			dprintk(VIDC_PROF, "FBD: FW needs output buffers\n");
		}
		if (inst->count.etb == inst->count.ebd)
			dprintk(VIDC_PROF, "FBD: FW needs input buffers\n");
		break;
	default:
		dprintk(VIDC_ERR, "Invalid state in debugfs: %d\n", e);
		break;
	}
}

