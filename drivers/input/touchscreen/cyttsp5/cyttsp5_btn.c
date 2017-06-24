/*
 * cyttsp5_btn.c
 * Cypress TrueTouch(TM) Standard Product V5 CapSense Reports Module.
 * For use with Cypress Txx5xx parts.
 * Supported parts include:
 * TMA5XX
 *
 * Copyright (C) 2012-2013 Cypress Semiconductor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Contact Cypress Semiconductor at www.cypress.com <ttdrivers@cypress.com>
 *
 */

#include "cyttsp5_regs.h"
int tsp_keycodes[2] = {
	KEY_MENU,
	KEY_BACK,
};

static void cyttsp5_btn_key_action(struct cyttsp5_btn_data *bd,
	int cur_btn, u8 cur_btn_mask, int num_btns, int new_btn_state)
{
	struct device *dev = bd->dev;
	struct cyttsp5_sysinfo *si = bd->si;
	int i;
	int btn;
	int cur_btn_state;
	int state_from_ic;

	cur_btn_state = new_btn_state == CY_BTN_PRESSED ? CY_BTN_RELEASED :
		CY_BTN_PRESSED;

	for (i = 0; i < num_btns; i++) {
		btn = cur_btn + i;
		if (!si->btn[btn].enabled)
			continue;
		state_from_ic = (cur_btn_mask >> (i * CY_BITS_PER_BTN))
				& CY_NUM_BTN_EVENT_ID;
		if (state_from_ic == new_btn_state &&
				si->btn[btn].state == cur_btn_state) {
			input_report_key(bd->input, tsp_keycodes[btn],
					new_btn_state);
			si->btn[btn].state = new_btn_state;
			input_sync(bd->input);
			dev_dbg(dev, "%s: btn=%d key_code=%d %s\n", __func__,
				btn, si->btn[btn].key_code,
				new_btn_state == CY_BTN_PRESSED ?
				"PRESSED" : "RELEASED");
		}
	}
	return;
}

static void cyttsp5_get_btn_touches(struct cyttsp5_btn_data *bd)
{
	enum cyttsp5_btn_state btn_state = CY_BTN_RELEASED;
	struct cyttsp5_sysinfo *si = bd->si;
	int num_cur_btn;
	int cur_btn;
	u8 cur_btn_mask;

	for (btn_state = CY_BTN_RELEASED; btn_state < CY_BTN_NUM_STATE;
		btn_state++) {
		num_cur_btn = si->num_btns;
		cur_btn = 0;
		if (num_cur_btn > 0) {
			cur_btn_mask = si->xy_data[0];
			cyttsp5_btn_key_action(bd, cur_btn,
				cur_btn_mask, num_cur_btn, btn_state);
		}
	}
	return;
}

static void cyttsp5_btn_lift_all(struct cyttsp5_btn_data *bd)
{
	struct cyttsp5_sysinfo *si = bd->si;
	if (si->num_btns == 0)
		return;

	si->xy_data[0] = 0;

	cyttsp5_get_btn_touches(bd);
}

#ifdef VERBOSE_DEBUG
static void cyttsp5_log_btn_data(struct cyttsp5_btn_data *bd)
{
	struct device *dev = bd->dev;
	struct cyttsp5_sysinfo *si = bd->si;
	int cur;
	int value;

	for (cur = 0; cur < si->num_btns; cur++) {
		bd->pr_buf[0] = 0;
		if (si->xy_data[0] & (1 << cur))
			value = 1;
		else
			value = 0;
		snprintf(bd->pr_buf, CY_MAX_PRBUF_SIZE, "btn_rec[%d]=0x", cur);
		snprintf(bd->pr_buf, CY_MAX_PRBUF_SIZE, "%s%X (%02X)",
			bd->pr_buf, value,
			le16_to_cpu(si->xy_data[1 + cur * 2]));

		dev_vdbg(dev, "%s: %s\n", __func__, bd->pr_buf);
	}
	return;
}
#endif

/* read xy_data for all current CapSense button touches */
static int cyttsp5_xy_worker(struct cyttsp5_btn_data *bd)
{
	struct cyttsp5_sysinfo *si = bd->si;

	/* extract button press/release touch information */
	if (si->num_btns > 0) {
		cyttsp5_get_btn_touches(bd);
#ifdef VERBOSE_DEBUG
		/* log button press/release touch information */
		cyttsp5_log_btn_data(bd);
#endif
	}

	return 0;
}

static int cyttsp5_btn_attention(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_btn_data *bd = &cd->bd;
	int rc;

	if (bd->si->xy_mode[2] != bd->si->desc.btn_report_id)
		return 0;

	/* core handles handshake */
	mutex_lock(&bd->btn_lock);
	rc = cyttsp5_xy_worker(bd);
	mutex_unlock(&bd->btn_lock);
	if (rc < 0)
		dev_err(dev, "%s: xy_worker error r=%d\n", __func__, rc);

	return rc;
}

static int cyttsp5_startup_attention(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_btn_data *bd = &cd->bd;
	struct cyttsp5_sysinfo *si = bd->si;
	int btn;

	mutex_lock(&bd->btn_lock);
	if (bd->si)
		cyttsp5_btn_lift_all(bd);

	for (btn = 0; btn < si->num_btns; btn++)
		bd->si->btn[btn].state = CY_BTN_RELEASED;
	mutex_unlock(&bd->btn_lock);

	return 0;
}

static int cyttsp5_btn_open(struct input_dev *input)
{
	struct device *dev = input->dev.parent;

	pm_runtime_get_sync(dev);

	dev_vdbg(dev, "%s: setup subscriptions\n", __func__);

	/* set up touch call back */
	_cyttsp5_subscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_BTN,
		cyttsp5_btn_attention, CY_MODE_OPERATIONAL);

	/* set up startup call back */
	_cyttsp5_subscribe_attention(dev, CY_ATTEN_STARTUP, CY_MODULE_BTN,
		cyttsp5_startup_attention, 0);

	return 0;
}

static void cyttsp5_btn_close(struct input_dev *input)
{
	struct device *dev = input->dev.parent;

	_cyttsp5_unsubscribe_attention(dev, CY_ATTEN_IRQ, CY_MODULE_BTN,
		cyttsp5_btn_attention, CY_MODE_OPERATIONAL);

	_cyttsp5_unsubscribe_attention(dev, CY_ATTEN_STARTUP, CY_MODULE_BTN,
		cyttsp5_startup_attention, 0);

	pm_runtime_put(dev);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cyttsp5_btn_early_suspend(struct early_suspend *h)
{
	struct cyttsp5_btn_data *bd =
		container_of(h, struct cyttsp5_btn_data, es);
	struct device *dev = bd->dev;

	pm_runtime_put_sync(dev);

	bd->is_suspended = true;
}

static void cyttsp5_btn_late_resume(struct early_suspend *h)
{
	struct cyttsp5_btn_data *bd =
		container_of(h, struct cyttsp5_btn_data, es);
	struct device *dev = bd->dev;

	pm_runtime_get_sync(dev);

	bd->is_suspended = false;
}
#endif

static int cyttsp5_setup_input_device(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_btn_data *bd = &cd->bd;
//	int i;
	int rc;

	dev_vdbg(dev, "%s: Initialize event signals\n", __func__);
	__set_bit(EV_KEY, bd->input->evbit);
	dev_vdbg(dev, "%s: Number of buttons %d\n", __func__, bd->si->num_btns);
#if 1
	set_bit(KEY_MENU, bd->input->keybit);
	set_bit(KEY_BACK, bd->input->keybit);
#else
	for (i = 0; i < bd->si->num_btns; i++) {
		dev_vdbg(dev, "%s: btn:%d keycode:%d\n",
			__func__, i, bd->si->btn[i].key_code);
		__set_bit(bd->si->btn[i].key_code, bd->input->keybit);
	}
#endif
	rc = input_register_device(bd->input);
	if (rc < 0)
		dev_err(dev, "%s: Error, failed register input device r=%d\n",
			__func__, rc);
	else
		bd->input_device_registered = true;

	return rc;
}

static int cyttsp5_setup_input_attention(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_btn_data *bd = &cd->bd;
	int rc;

	bd->si = _cyttsp5_request_sysinfo(dev);
	if (!bd->si)
		return -1;

	rc = cyttsp5_setup_input_device(dev);

	_cyttsp5_unsubscribe_attention(dev, CY_ATTEN_STARTUP, CY_MODULE_BTN,
		cyttsp5_setup_input_attention, 0);

	return rc;
}

int cyttsp5_btn_probe(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_btn_data *bd = &cd->bd;
	struct cyttsp5_platform_data *pdata = dev_get_platdata(dev);
	struct cyttsp5_btn_platform_data *btn_pdata;
	int rc = 0;

	if (!pdata || !pdata->btn_pdata) {
		dev_err(dev, "%s: Missing platform data\n", __func__);
		rc = -ENODEV;
		goto error_no_pdata;
	}
	btn_pdata = pdata->btn_pdata;

	mutex_init(&bd->btn_lock);
	bd->dev = dev;
	bd->pdata = btn_pdata;

	/* Create the input device and register it. */
	dev_vdbg(dev, "%s: Create the input device and register it\n",
		__func__);
	bd->input = input_allocate_device();
	if (!bd->input) {
		dev_err(dev, "%s: Error, failed to allocate input device\n",
			__func__);
		rc = -ENOSYS;
		goto error_alloc_failed;
	}

	if (bd->pdata->inp_dev_name)
		bd->input->name = bd->pdata->inp_dev_name;
	else
		bd->input->name = CYTTSP5_BTN_NAME;
	scnprintf(bd->phys, sizeof(bd->phys)-1, "%s", CYTTSP5_BTN_NAME);
	bd->input->phys = bd->phys;
	bd->input->dev.parent = bd->dev;
	bd->input->open = cyttsp5_btn_open;
	bd->input->close = cyttsp5_btn_close;
	input_set_drvdata(bd->input, bd);

	/* get sysinfo */
	bd->si = _cyttsp5_request_sysinfo(dev);

	if (bd->si) {
		rc = cyttsp5_setup_input_device(dev);
		if (rc)
			goto error_init_input;
	} else {
		dev_err(dev, "%s: Fail get sysinfo pointer from core p=%p\n",
			__func__, bd->si);
		_cyttsp5_subscribe_attention(dev, CY_ATTEN_STARTUP,
			CY_MODULE_BTN, cyttsp5_setup_input_attention, 0);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	bd->es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	bd->es.suspend = cyttsp5_btn_early_suspend;
	bd->es.resume = cyttsp5_btn_late_resume;
	register_early_suspend(&bd->es);
#endif

	return 0;

error_init_input:
	input_free_device(bd->input);
error_alloc_failed:
error_no_pdata:
	dev_err(dev, "%s failed.\n", __func__);
	return rc;
}
EXPORT_SYMBOL(cyttsp5_btn_probe);

int cyttsp5_btn_release(struct device *dev)
{
	struct cyttsp5_core_data *cd = dev_get_drvdata(dev);
	struct cyttsp5_btn_data *bd = &cd->bd;

#ifdef CONFIG_HAS_EARLYSUSPEND
	/*
	 * This check is to prevent pm_runtime usage_count drop below zero
	 * because of removing the module while in suspended state
	 */
	if (bd->is_suspended)
		pm_runtime_get_noresume(dev);

	unregister_early_suspend(&bd->es);
#endif

	if (bd->input_device_registered) {
		input_unregister_device(bd->input);
	} else {
		input_free_device(bd->input);
		_cyttsp5_unsubscribe_attention(dev, CY_ATTEN_STARTUP,
			CY_MODULE_BTN, cyttsp5_setup_input_attention, 0);
	}

	return 0;
}
EXPORT_SYMBOL(cyttsp5_btn_release);
