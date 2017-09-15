/*
 * @file tz_iccc.c
 * @brief Kernel API code for tz_iccc
 * Copyright (c) 2015, Samsung Electronics Corporation. All rights reserved.
 */
#include <asm/uaccess.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include "tz_iccc.h"
#include <linux/security/iccc_interface.h>
#include <linux/qseecom.h>

#include <linux/init.h>
#include <linux/types.h>
#include <linux/proc_fs.h>

#if defined(CONFIG_SECURITY_SELINUX)
#include <linux/selinux.h>
#endif

/* ICCC implementation for kernel */

int is_iccc_ready;
#define DRIVER_DESC "A kernel module to read boot_completed status"

int tima_iccc_load(char* appname,struct qseecom_handle** handle)
{
	int ret=ICCC_SUCCESS;
	int qsee_ret = 0;
	struct qseecom_handle* q_iccc_handle = NULL;

	*handle = NULL;
	/* start the iccc tzapp only when it is not loaded. */
	qsee_ret = qseecom_start_app(&q_iccc_handle, appname, ICCC_QSEE_BUFFER_LENGTH);

	if ( NULL == q_iccc_handle ) {
		/* q_iccc_handle is still NULL. It seems we couldn't start iccc tzapp. */
		pr_err("TIMA: iccc--cannot get tzapp handle from kernel.\n");
		ret = ICCC_FAILURE; /* iccc authentication failed. */
	}

	if (qsee_ret) {
		/* Another way for iccc tzapp loading to fail. */
		pr_err("TIMA: iccc--cannot load tzapp from kernel; qsee_ret =  %d.\n", qsee_ret);
		ret = ICCC_FAILURE;
	}

	if(ret == ICCC_SUCCESS)
	{
		*handle = q_iccc_handle;
	}
	return ret;
}

int tima_iccc_terminate(struct qseecom_handle** q_iccc_handle)
{
	int qsee_ret = 0;
	qsee_ret = qseecom_shutdown_app(q_iccc_handle);

	if ( qsee_ret ) {
		pr_err("TIMA: iccc--failed to shut down the tzapp.\n");
	}
	else
		*q_iccc_handle = NULL;

	return qsee_ret;
}

uint32_t Iccc_SaveData_Kernel(uint32_t type, uint32_t value)
{
	char app_name[MAX_APP_NAME_SIZE]={0};
	int ret=0;
        tciMessage_t * iccc_req = NULL;
        tciMessage_t * iccc_rsp = NULL;
	int req_len = 0, rsp_len = 0;
	tciMessage_t *msg;
	int qsee_ret = 0; /* value used to capture qsee return state */
	struct qseecom_handle *q_iccc_handle = NULL;

	printk(KERN_ERR "inside Iccc_SaveData_Kernel \n");

	if (!is_iccc_ready) {
		ret = ICCC_PERMISSION_DENIED;
		pr_err("%s: Not ready! type:%#x, ret:%d\n", __func__, type, ret);
		goto iccc_err_ret;
	}

	if (ICCC_SECTION_TYPE(type) == BL_ICCC_TYPE_START ||
		ICCC_SECTION_TYPE(type) == SYS_ICCC_TYPE_START)
	{
		ret=ICCC_PERMISSION_DENIED;
		pr_err("iccc--Write permission is denied on type %x, ret = %d.\n", type, ret);
		goto iccc_err_ret;
	}

	/**
	 * selinux param is updated by both TZ(by PKM) as well as kernel so that
	 * that even if PKM is disabled, selinux will still be updated by kernel.
	 */
	if (ICCC_SECTION_TYPE(type) == TA_ICCC_TYPE_START && type != SELINUX_STATUS)
	{
		ret=ICCC_PERMISSION_DENIED;
		pr_err("iccc--Write permission is denied on type %x, ret = %d.\n", type, ret);
		goto iccc_err_ret;
	}

	snprintf(app_name, MAX_APP_NAME_SIZE, "%s", ICCC_TZAPP_NAME);

	if (tima_iccc_load(app_name,&q_iccc_handle)) {
		pr_err("%s: tima_iccc_load() error!\n", __func__);
		/* Another way for iccc tzapp loading to fail. */
		q_iccc_handle = NULL; /* Do we have a memory leak this way? */
		ret = ICCC_FAILURE; /* iccc authentication failed. */
		goto iccc_err_ret; /* leave the function now. */
	}

	iccc_req = (tciMessage_t *) q_iccc_handle->sbuf;
	req_len = sizeof(tciMessage_t);
	if (req_len & QSEECOM_ALIGN_MASK)
		req_len = QSEECOM_ALIGN(req_len);

	/* prepare the response buffer */
	iccc_rsp =(tciMessage_t *)(q_iccc_handle->sbuf + req_len);

	rsp_len = sizeof(tciMessage_t);
	if (rsp_len & QSEECOM_ALIGN_MASK)
		rsp_len = QSEECOM_ALIGN(rsp_len);

	if ((rsp_len + req_len) > ICCC_QSEE_BUFFER_LENGTH) {
		pr_err("TIMA: iccc--in suffcient buffer length: %d\n", rsp_len + req_len);
		ret = ICCC_FAILURE;
		goto iccc_err_ret;
	}

	msg = (tciMessage_t *)iccc_req;
	msg->header.id = CMD_ICCC_SAVEDATA_KERN;
	msg->payload.generic.content.iccc_req.cmd_id = CMD_ICCC_SAVEDATA_KERN;
	msg->payload.generic.content.iccc_req.type = type;
	msg->payload.generic.content.iccc_req.value = value;

#ifdef CONFIG_64BIT
	pr_warn("TIMA: iccc--send cmd (%s) cmdlen(%lx:%d), rsplen(%lx:%d) id 0x%08X, \
                type 0x%08X, value %08d, req (0x%16lX), rsp(0x%16lX)\n", \
		app_name, sizeof(tciMessage_t), req_len, sizeof(tciMessage_t), rsp_len, \
		msg->header.id,type,value, (unsigned long)iccc_req, (unsigned long)iccc_rsp);
#else
	pr_warn("TIMA: iccc--send cmd (%s) cmdlen(%d:%d), rsplen(%d:%d) id 0x%08X, \
                type 0x%08X, value %08d, req (0x%08X), rsp(0x%08X)\n", \
		app_name, sizeof(tciMessage_t), req_len, sizeof(tciMessage_t), rsp_len, \
		msg->header.id,type,value, (int)iccc_req, (int)iccc_rsp);
#endif

	qseecom_set_bandwidth(q_iccc_handle, true);
	qsee_ret = qseecom_send_command(q_iccc_handle, iccc_req, req_len, iccc_rsp, rsp_len);
	qseecom_set_bandwidth(q_iccc_handle, false);

	if (qsee_ret) {
		pr_err("TIMA: iccc--failed to send cmd to qseecom; qsee_ret = %d.\n", qsee_ret);
		pr_warn("TIMA: iccc--shutting down the tzapp.\n");
		ret = ICCC_FAILURE;
		goto iccc_err_ret;
	}

	if (iccc_rsp->payload.generic.content.iccc_rsp.ret == ICCC_SUCCESS) {
		pr_info("TIMA: iccc--Iccc_SaveData_Kernel sucessfully\n");
		ret = ICCC_SUCCESS;
	}
	else
	{
		ret = ICCC_FAILURE;
		pr_err("TIMA: iccc-- Iccc_SaveData_Kernel failed (%d)\n",msg->payload.generic.content.iccc_rsp.ret);
		goto iccc_err_ret;
	}

iccc_err_ret:
	if(q_iccc_handle)
		tima_iccc_terminate(&q_iccc_handle);
	return ret;
}

uint32_t Iccc_ReadData_Kernel(uint32_t type, uint32_t *value)
{
	char app_name[MAX_APP_NAME_SIZE]={0};
	int ret=0;
        tciMessage_t * iccc_req = NULL;
        tciMessage_t * iccc_rsp = NULL;
	int req_len = 0, rsp_len = 0;
	tciMessage_t *msg;
	int qsee_ret = 0; /* value used to capture qsee return state */
	struct qseecom_handle *q_iccc_handle = NULL;

	printk(KERN_ERR "inside Iccc_ReadData_Kernel \n");

	if (!is_iccc_ready) {
		ret = ICCC_PERMISSION_DENIED;
		pr_err("%s: Not ready! type:%#x, ret:%d\n", __func__, type, ret);
		goto iccc_err_ret;
	}

	snprintf(app_name, MAX_APP_NAME_SIZE, "%s", ICCC_TZAPP_NAME);
	if (tima_iccc_load(app_name,&q_iccc_handle)) {
		/* Another way for iccc tzapp loading to fail. */
		q_iccc_handle = NULL; /* Do we have a memory leak this way? */
		ret = -1; /* iccc authentication failed. */
		goto iccc_err_ret; /* leave the function now. */
	}

	iccc_req = (tciMessage_t *) q_iccc_handle->sbuf;
	req_len = sizeof(tciMessage_t);
	if (req_len & QSEECOM_ALIGN_MASK)
		req_len = QSEECOM_ALIGN(req_len);

	/* prepare the response buffer */
	iccc_rsp =(tciMessage_t *)(q_iccc_handle->sbuf + req_len);

	rsp_len = sizeof(tciMessage_t);
	if (rsp_len & QSEECOM_ALIGN_MASK)
		rsp_len = QSEECOM_ALIGN(rsp_len);

	if ((rsp_len + req_len) > ICCC_QSEE_BUFFER_LENGTH) {
		pr_err("TIMA: iccc--in suffcient buffer length: %d\n", rsp_len + req_len);
		ret = ICCC_FAILURE;
		goto iccc_err_ret;
	}

	msg = (tciMessage_t *)iccc_req;
	msg->header.id = CMD_ICCC_READDATA_KERN;
	msg->payload.generic.content.iccc_req.cmd_id = CMD_ICCC_READDATA_KERN;
	msg->payload.generic.content.iccc_req.type = type;
	msg->payload.generic.content.iccc_req.value = *value;

#ifdef CONFIG_64BIT
	pr_warn("TIMA: iccc--send cmd (%s) cmdlen(%lx:%d), rsplen(%lx:%d) id 0x%08X, \
                type 0x%08X, value %08d, req (0x%16lX), rsp(0x%16lX)\n", \
		app_name, sizeof(tciMessage_t), req_len, sizeof(tciMessage_t), rsp_len, \
		msg->header.id,type,*value, (unsigned long)iccc_req, (unsigned long)iccc_rsp);
#else
	pr_warn("TIMA: iccc--send cmd (%s) cmdlen(%d:%d), rsplen(%d:%d) id 0x%08X, \
                type 0x%08X, value %08d, req (0x%08X), rsp(0x%08X)\n", \
		app_name, sizeof(tciMessage_t), req_len, sizeof(tciMessage_t), rsp_len, \
		msg->header.id,type,*value, (int)iccc_req, (int)iccc_rsp);
#endif

	qseecom_set_bandwidth(q_iccc_handle, true);
	qsee_ret = qseecom_send_command(q_iccc_handle, iccc_req, req_len, iccc_rsp, rsp_len);
	qseecom_set_bandwidth(q_iccc_handle, false);

	if (qsee_ret) {
		pr_err("TIMA: iccc--failed to send cmd to qseecom; qsee_ret = %d.\n", qsee_ret);
		pr_warn("TIMA: iccc--shutting down the tzapp.\n");
		ret = ICCC_FAILURE;
		goto iccc_err_ret;
	}

	if (iccc_rsp->payload.generic.content.iccc_rsp.ret == ICCC_SUCCESS) {
		pr_info("TIMA: iccc--Iccc_ReadData_Kernel sucessfully\n");
		ret = ICCC_SUCCESS;
	}
	else
	{
		ret = ICCC_FAILURE;
		pr_err("TIMA: iccc-- Iccc_ReadData_Kernel failed (%d)\n",iccc_rsp->payload.generic.content.iccc_rsp.ret);
		goto iccc_err_ret;
	}

	pr_err("ICCC Info type:0x%08x value:%d",type,iccc_rsp->payload.generic.content.iccc_rsp.value);
	*value = iccc_rsp->payload.generic.content.iccc_rsp.value;
	ret = ICCC_SUCCESS;

iccc_err_ret:
	if(q_iccc_handle)
		tima_iccc_terminate(&q_iccc_handle);
	return ret;
}

static ssize_t iccc_write(struct file *fp, const char __user *buf, size_t len, loff_t *off)
{
	printk(KERN_INFO "%s:\n", __func__);

	is_iccc_ready = 1;

#if defined(CONFIG_SECURITY_SELINUX)
	printk(KERN_INFO "%s: selinux_enabled:%d, selinux_enforcing:%d\n",
		__func__, selinux_is_enabled(), selinux_is_enforcing());
	if (selinux_is_enabled() && selinux_is_enforcing())
		Iccc_SaveData_Kernel(SELINUX_STATUS, 0x0);
	else
		Iccc_SaveData_Kernel(SELINUX_STATUS, 0x1);
#endif

	// len bytes successfully written 
	return len;
}

static const struct file_operations iccc_proc_fops = {
	.write = iccc_write,
};

static int __init iccc_init(void)
{
	printk(KERN_INFO"%s:\n", __func__);

	if (proc_create("iccc_ready", 0644, NULL, &iccc_proc_fops) == NULL) {
		printk(KERN_ERR"%s: proc_create() failed\n", __func__);
		return -1;
	}

	printk(KERN_INFO"%s: registered /proc/iccc_ready interface\n", __func__);

	return 0;
}

static void __exit iccc_exit(void)
{
	printk(KERN_INFO"deregistering /proc/iccc_boot_completed interface\n");
	remove_proc_entry("iccc_ready", NULL);
}

module_init(iccc_init);
module_exit(iccc_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
