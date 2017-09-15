#include "escore.h"
#include "escore-i2c.h"
#include "escore-i2s.h"

/* Send a single command to the chip.
 *
 * If the SR (suppress response bit) is NOT set, will read the
 * response and cache it the driver object retrieve with escore_resp().
 *
 * Returns:
 * 0 - on success.
 * EITIMEDOUT - if the chip did not respond in within the expected time.
 * E* - any value that can be returned by the underlying HAL.
 */
int escore_cmd(struct escore_priv *escore, u32 cmd)
{
	int sr;
	int err;
	u32 resp;

	BUG_ON(!escore);
	sr = cmd & BIT(28);

	err = escore->cmd(escore, cmd, sr, &resp);
	if (err || sr)
		return err;

	if (resp == 0) {
		err = -ETIMEDOUT;
		dev_err(escore->dev, "no response to command 0x%08x\n", cmd);
	} else {
		escore->last_response = resp;
		get_monotonic_boottime(&escore->last_resp_time);
	}
	return err;
}

int escore_write_block(struct escore_priv *escore, const u32 *cmd_block)
{
	int rc = 0;

	pr_debug("%s(): pm_runtime_get_sync()\n", __func__);
	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&escore->api_mutex);
	while (*cmd_block != 0xffffffff) {
		escore_cmd(escore, *cmd_block);
		usleep_range(1000, 1000);
		cmd_block++;
	}
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&escore->api_mutex);
	pr_debug("%s(): pm_runtime_put_autosuspend()\n", __func__);

	return rc;
}

unsigned int escore_read(struct snd_soc_codec *codec,
			       unsigned int reg)
{
	struct escore_priv *escore = &escore_priv;
	struct escore_api_access *api_access;
	u32 api_word[2] = {0};
	unsigned int msg_len;
	unsigned int value;
	int rc;

	if (reg > escore->api_addr_max) {
		pr_err("%s(): invalid address = 0x%04x\n", __func__, reg);
		return -EINVAL;
	}

	api_access = &escore->escore_api_access[reg];
	msg_len = api_access->read_msg_len;
	memcpy((char *)api_word, (char *)api_access->read_msg, msg_len);

	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&escore->api_mutex);
	rc = escore_cmd(escore, api_word[0]);
	if (rc < 0) {
		pr_err("%s(): escore_xxxx_write()", __func__);
		pr_debug("%s(): mutex unlock\n", __func__);
		mutex_unlock(&escore->api_mutex);
		return rc;
	}
	api_word[0] = escore->last_response;
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&escore->api_mutex);

	value = api_word[0] & 0xffff;

	return value;
}

int escore_write(struct snd_soc_codec *codec, unsigned int reg,
		       unsigned int value)
{
	struct escore_priv *escore = &escore_priv;
	struct escore_api_access *api_access;
	u32 api_word[2] = {0};
	int msg_len;
	unsigned int val_mask;
	int i;
	int rc = 0;

	if (reg > escore->api_addr_max) {
		pr_err("%s(): invalid address = 0x%04x\n", __func__, reg);
		return -EINVAL;
	}

	pr_debug("%s(): reg=%08x val=%d\n", __func__, reg, value);

	api_access = &escore->escore_api_access[reg];
	msg_len = api_access->write_msg_len;
	val_mask = (1 << get_bitmask_order(api_access->val_max)) - 1;
	memcpy((char *)api_word, (char *)api_access->write_msg, msg_len);

	switch (msg_len) {
	case 8:
		api_word[1] |= (val_mask & value);
		break;
	case 4:
		api_word[0] |= (val_mask & value);
		break;
	}

	pr_debug("%s(): mutex lock\n", __func__);
	mutex_lock(&escore->api_mutex);
	for (i = 0; i < msg_len / 4; i++) {
		rc = escore_cmd(escore, api_word[i]);
		if (rc < 0) {
			pr_err("%s(): escore_cmd()", __func__);
			pr_info("%s(): mutex unlock\n", __func__);
			mutex_unlock(&escore->api_mutex);
			return rc;
		}
	}
	pr_debug("%s(): mutex unlock\n", __func__);
	mutex_unlock(&escore->api_mutex);

	return rc;
}

int escore_read_and_clear_intr(struct escore_priv *escore)
{
	int value;
	int rc;
	struct snd_soc_codec *codec = escore->codec;

	pr_debug("%s()\n", __func__);

	value = escore_read(codec, escore->regs->get_intr_status);
	if (value < 0) {
		pr_err("%s(): Reading System Interrupt Status failed\n",
				__func__);
		goto status_read_error;
	}

	rc = escore_write(codec, escore->regs->clear_intr_status, value);
	if (rc < 0) {
		pr_err("%s(): Clearing interrupt status failed\n",
				__func__);
		goto int_clear_error;
	}

status_read_error:
	return value;
int_clear_error:
	return rc;

}

int escore_accdet_config(struct escore_priv *escore, int enable)
{
	int rc;
	struct snd_soc_codec *codec = escore->codec;

	pr_debug("%s()\n", __func__);

	rc = escore_write(codec, escore->regs->accdet_config, enable);
	if (rc < 0)
		pr_err("Accdet detection enabling failed\n");

	return rc;
}
EXPORT_SYMBOL_GPL(escore_accdet_config);

int escore_btndet_config(struct escore_priv *escore, int enable)
{
	int rc;
	struct snd_soc_codec *codec = escore->codec;
	struct esxxx_accdet_config *accdet_cfg = &escore->pdata->accdet_cfg;

	pr_debug("%s()\n", __func__);
	rc = escore_write(codec, escore->regs->enable_btndet, enable);
	if (rc < 0) {
		pr_err("Button detection enabling failed\n");
		goto btndet_config_error;
	}

	if (enable) {
		/* Enable serial button config */
		if (accdet_cfg->btn_serial_cfg != -1) {
			rc = escore_write(codec,
					escore->regs->btn_serial_cfg,
					accdet_cfg->btn_serial_cfg);
			if (rc < 0) {
				pr_err("Serial button config failed\n");
				goto btndet_config_error;
			}
		}

		/* Enable parallel button config */
		if (accdet_cfg->btn_parallel_cfg != -1) {
			rc = escore_write(codec,
					escore->regs->btn_parallel_cfg,
					accdet_cfg->btn_parallel_cfg);
			if (rc < 0) {
				pr_err("Parallel button config failed\n");
				goto btndet_config_error;
			}
		}

		/* Set button detection rate */
		if (accdet_cfg->btn_detection_rate != -1) {
			rc = escore_write(codec,
					escore->regs->btn_detection_rate,
					accdet_cfg->btn_detection_rate);
			if (rc < 0) {
				pr_err("Set button detection rate failed\n");
				goto btndet_config_error;
			}
		}

		/* Set settling time config for button press */
		if (accdet_cfg->btn_press_settling_time != -1) {
			rc = escore_write(codec,
					escore->regs->btn_press_settling_time,
					accdet_cfg->btn_press_settling_time);
			if (rc < 0) {
				pr_err("Set button settling time failed\n");
				goto btndet_config_error;
			}
		}

		/* Set bounce time config for button press */
		if (accdet_cfg->btn_bounce_time != -1) {
			rc = escore_write(codec,
					escore->regs->btn_bounce_time,
					accdet_cfg->btn_bounce_time);
			if (rc < 0) {
				pr_err("Set button bounce time failed\n");
				goto btndet_config_error;
			}
		}

		/*
		 * Sets the time duration for a button press necessary
		 * to classify a press detection event as a LONG button
		 * press
		 */

		if (accdet_cfg->btn_long_press_time != -1) {
			rc = escore_write(codec,
					escore->regs->btn_long_press_time,
					accdet_cfg->btn_long_press_time);
		if (rc < 0) {
			pr_err("Long Button Press config failed\n");
			goto btndet_config_error;
			}
		}
	}

btndet_config_error:
	return rc;

}
EXPORT_SYMBOL_GPL(escore_btndet_config);

int escore_process_accdet(struct escore_priv *escore)
{
	int value;
	struct snd_soc_codec *codec = escore->codec;

	pr_debug("%s()\n", __func__);

	/* Find out type of accessory using Get Accessory Detect
	 * Status Command */
	value = escore_read(codec, escore->regs->accdet_status);
	if (value < 0) {
		pr_err("%s(): Reading accessory detect Status failed\n",
				__func__);
		goto accdet_error;
	}

	if (ES_IS_HEADSET(value)) {
		pr_info("%s(): LRGM Headset\n", __func__);
		value = escore_btndet_config(escore, ES_BTNDET_ENABLE);
		if (value < 0) {
			pr_err("%s(): Enabling button detection failed\n",
					__func__);
			goto accdet_error;
		}

	} else if (ES_IS_LRG_HEADPHONE(value))
		pr_info("%s(): LRG Headphone\n", __func__);
	else
		pr_info("Unknown Accessory detected\n");

accdet_error:
	return value;
}
EXPORT_SYMBOL_GPL(escore_process_accdet);

/*
 * Placeholder for digital chip related interrupts
 */
void escore_process_digital_intr(struct escore_priv *escore)
{
	if (!escore->process_digital)
		return;

	/* TODO: Add the generic digital interrupt handling */
}

/*
 * Processes the various analog interrupts. Detects the type of
 * accessory plugged in (either headphone or headset) and configures
 * the accessory if required.
 *
 * TODO: Report the jack plug/unplug event to userspace
 */
void escore_process_analog_intr(struct escore_priv *escore)
{
	int rc = 0;
	int value;

	pr_debug("%s()\n", __func__);
	if (!escore->process_analog)
		goto process_analog_intr_error;

	value = escore_read_and_clear_intr(escore);
	if (ES_IS_PLUG_EVENT(value)) {

		pr_info("%s(): Plug event\n", __func__);
		/* Enable accessory detection */
		rc = escore_accdet_config(escore, ES_ACCDET_ENABLE);
		if (rc < 0) {
			pr_err("%s(): Enabling accessory detection failed\n",
					__func__);
			goto process_analog_intr_error;
		}
	} else if (ES_IS_UNPLUG_EVENT(value)) {

		pr_info("%s(): Unplug event\n", __func__);
		/* Disable button detection */
		rc = escore_btndet_config(escore, ES_BTNDET_DISABLE);
		if (rc < 0) {
			pr_err("%s(): Disabling button detection failed\n",
					__func__);
			goto process_analog_intr_error;
		}

		/* Disable accessory detection */
		rc = escore_accdet_config(escore, ES_ACCDET_DISABLE);
		if (rc < 0) {
			pr_err("%s(): Disabling accessory detection failed\n",
					__func__);
			goto process_analog_intr_error;
		}
	} else if (ES_IS_ACCDET_EVENT(value)) {

		pr_info("%s(): Accdet event\n", __func__);
		/* Process accessory detection */
		rc = escore_process_accdet(escore);
		if (rc < 0) {
			pr_err("%s(): Processing accessory detection failed\n",
					__func__);
			goto process_analog_intr_error;
		}

	} else if (ES_IS_BTN_PRESS_EVENT(value)) {

		if (ES_IS_SHORT_BTN_PARALLEL_PRESS(value))
			pr_info("%s(): Short button parallel press event\n",
					__func__);
		else if (ES_IS_SHORT_BTN_SERIAL_PRESS(value))
			pr_info("%s(): Short button serial press event\n",
					__func__);
		else if (ES_IS_LONG_BTN_PARALLEL_PRESS(value))
			pr_info("%s(): Long button parallel press event\n",
					__func__);
		else if (ES_IS_LONG_BTN_SERIAL_PRESS(value))
			pr_info("%s(): Long button serial press event\n",
					__func__);
	} else
		pr_info("%s(): Unknown Interrupt %x\n", __func__, value);

process_analog_intr_error:
	return;
}


/*
 * Generic ISR for Audience chips. It is divided mainly into two parts to
 * process interrupts for:
 * 1) chips containing codec
 * 2) chips only having digital component
 */

irqreturn_t escore_irq_work(int irq, void *data)
{
	struct escore_priv *escore = data;

	escore_process_digital_intr(escore);

	escore_process_analog_intr(escore);

	return IRQ_HANDLED;
}

