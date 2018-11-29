/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spmi.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/log2.h>
#include <linux/qpnp/power-on.h>
#ifdef CONFIG_SEC_DEBUG
#include <mach/sec_debug.h>
#endif

#define PMIC_VER_8941           0x01
#define PMIC_VERSION_REG        0x0105
#define PMIC_VERSION_REV4_REG   0x0103

#define PMIC8941_V1_REV4	0x01
#define PMIC8941_V2_REV4	0x02
#define PON_REV2_VALUE		0x00

#if defined(CONFIG_SEC_PATEK_PROJECT) || defined(CONFIG_SEC_S_PROJECT)
	static int check_pkey_press;
#endif

extern struct class *sec_class;

/* Common PNP defines */
#define QPNP_PON_REVISION2(base)		(base + 0x01)

/* PON common register addresses */
#define QPNP_PON_RT_STS(base)			(base + 0x10)
#define QPNP_PON_PULL_CTL(base)			(base + 0x70)
#define QPNP_PON_DBC_CTL(base)			(base + 0x71)

/* PON/RESET sources register addresses */
#define QPNP_PON_REASON1(base)			(base + 0x8)
#define QPNP_PON_WARM_RESET_REASON1(base)	(base + 0xA)
#define QPNP_PON_WARM_RESET_REASON2(base)	(base + 0xB)
#define QPNP_POFF_REASON1(base)			(base + 0xC)
#define QPNP_PON_KPDPWR_S1_TIMER(base)		(base + 0x40)
#define QPNP_PON_KPDPWR_S2_TIMER(base)		(base + 0x41)
#define QPNP_PON_KPDPWR_S2_CNTL(base)		(base + 0x42)
#define QPNP_PON_KPDPWR_S2_CNTL2(base)		(base + 0x43)
#define QPNP_PON_RESIN_S1_TIMER(base)		(base + 0x44)
#define QPNP_PON_RESIN_S2_TIMER(base)		(base + 0x45)
#define QPNP_PON_RESIN_S2_CNTL(base)		(base + 0x46)
#define QPNP_PON_RESIN_S2_CNTL2(base)		(base + 0x47)
#define QPNP_PON_KPDPWR_RESIN_S1_TIMER(base)	(base + 0x48)
#define QPNP_PON_KPDPWR_RESIN_S2_TIMER(base)	(base + 0x49)
#define QPNP_PON_KPDPWR_RESIN_S2_CNTL(base)	(base + 0x4A)
#define QPNP_PON_KPDPWR_RESIN_S2_CNTL2(base)	(base + 0x4B)
#define QPNP_PON_PS_HOLD_RST_CTL(base)		(base + 0x5A)
#define QPNP_PON_PS_HOLD_RST_CTL2(base)		(base + 0x5B)
#define QPNP_PON_WD_RST_S2_CTL(base)		(base + 0x56)
#define QPNP_PON_WD_RST_S2_CTL2(base)		(base + 0x57)
#define QPNP_PON_S3_SRC(base)			(base + 0x74)
#define QPNP_PON_S3_DBC_CTL(base)		(base + 0x75)
#define QPNP_PON_TRIGGER_EN(base)		(base + 0x80)

#define QPNP_PON_WARM_RESET_TFT			BIT(4)

#define QPNP_PON_RESIN_PULL_UP			BIT(0)
#define QPNP_PON_KPDPWR_PULL_UP			BIT(1)
#define QPNP_PON_CBLPWR_PULL_UP			BIT(2)
#define QPNP_PON_S2_CNTL_EN			BIT(7)
#define QPNP_PON_S2_RESET_ENABLE		BIT(7)
#define QPNP_PON_DELAY_BIT_SHIFT		6

#define QPNP_PON_S1_TIMER_MASK			(0xF)
#define QPNP_PON_S2_TIMER_MASK			(0x7)
#define QPNP_PON_S2_CNTL_TYPE_MASK		(0xF)

#define QPNP_PON_DBC_DELAY_MASK			(0x7)
#define QPNP_PON_KPDPWR_N_SET			BIT(0)
#define QPNP_PON_RESIN_N_SET			BIT(1)
#define QPNP_PON_CBLPWR_N_SET			BIT(2)
#define QPNP_PON_RESIN_BARK_N_SET		BIT(4)
#define QPNP_PON_KPDPWR_RESIN_BARK_N_SET	BIT(5)

#define QPNP_PON_WD_EN			BIT(7)
#define QPNP_PON_RESET_EN			BIT(7)
#define QPNP_PON_POWER_OFF_MASK			0xF

#define QPNP_PON_S3_SRC_KPDPWR			0
#define QPNP_PON_S3_SRC_RESIN			1
#define QPNP_PON_S3_SRC_KPDPWR_AND_RESIN	2
#define QPNP_PON_S3_SRC_KPDPWR_OR_RESIN		3
#define QPNP_PON_S3_SRC_MASK			0x3

/* Ranges */
#define QPNP_PON_S1_TIMER_MAX			10256
#define QPNP_PON_S2_TIMER_MAX			2000
#define QPNP_PON_S3_TIMER_SECS_MAX		128
#define QPNP_PON_S3_DBC_DELAY_MASK		0x07
#define QPNP_PON_RESET_TYPE_MAX			0xF
#define PON_S1_COUNT_MAX			0xF
#define QPNP_PON_MIN_DBC_US			(USEC_PER_SEC / 64)
#define QPNP_PON_MAX_DBC_US			(USEC_PER_SEC * 2)

#define QPNP_KEY_STATUS_DELAY			msecs_to_jiffies(250)

enum pon_type {
	PON_KPDPWR,
	PON_RESIN,
	PON_CBLPWR,
	PON_KPDPWR_RESIN,
};

struct qpnp_pon_config {
	u32 pon_type;
	u32 support_reset;
#ifdef CONFIG_SEC_PM
	u32 disable_reset;
#endif
	u32 key_code;
	u32 s1_timer;
	u32 s2_timer;
	u32 s2_type;
	u32 pull_up;
	u32 state_irq;
	u32 bark_irq;
	u16 s2_cntl_addr;
	u16 s2_cntl2_addr;
	bool use_bark;
};

struct qpnp_pon {
	struct spmi_device *spmi;
	struct input_dev *pon_input;
	struct qpnp_pon_config *pon_cfg;
	int num_pon_config;
	int powerkey_state;
	u16 base;
	struct delayed_work bark_work;
};

static struct qpnp_pon *sys_reset_dev;
#ifdef CONFIG_SEC_PM_DEBUG
static int wake_enabled;
static int reset_enabled;
#endif

static u32 s1_delay[PON_S1_COUNT_MAX + 1] = {
	0 , 32, 56, 80, 138, 184, 272, 408, 608, 904, 1352, 2048,
	3072, 4480, 6720, 10256
};

static const char * const qpnp_pon_reason[] = {
	[0] = "Triggered from Hard Reset",
	[1] = "Triggered from SMPL (sudden momentary power loss)",
	[2] = "Triggered from RTC (RTC alarm expiry)",
	[3] = "Triggered from DC (DC charger insertion)",
	[4] = "Triggered from USB (USB charger insertion)",
	[5] = "Triggered from PON1 (secondary PMIC)",
	[6] = "Triggered from CBL (external power supply)",
	[7] = "Triggered from KPD (power key press)",
};

static const char * const qpnp_poff_reason[] = {
	[0] = "Triggered from SOFT (Software)",
	[1] = "Triggered from PS_HOLD (PS_HOLD/MSM controlled shutdown)",
	[2] = "Triggered from PMIC_WD (PMIC watchdog)",
	[3] = "Triggered from GP1 (Keypad_Reset1)",
	[4] = "Triggered from GP2 (Keypad_Reset2)",
	[5] = "Triggered from KPDPWR_AND_RESIN"
		"(Simultaneous power key and reset line)",
	[6] = "Triggered from RESIN_N (Reset line/Volume Down Key)",
	[7] = "Triggered from KPDPWR_N (Long Power Key hold)",
	[8] = "N/A",
	[9] = "N/A",
	[10] = "N/A",
	[11] = "Triggered from CHARGER (Charger ENUM_TIMER, BOOT_DONE)",
	[12] = "Triggered from TFT (Thermal Fault Tolerance)",
	[13] = "Triggered from UVLO (Under Voltage Lock Out)",
	[14] = "Triggered from OTST3 (Overtemp)",
	[15] = "Triggered from STAGE3 (Stage 3 reset)",
};

static int
qpnp_pon_masked_write(struct qpnp_pon *pon, u16 addr, u8 mask, u8 val)
{
	int rc;
	u8 reg;

	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
							addr, &reg, 1);
	if (rc) {
		dev_err(&pon->spmi->dev,
			"Unable to read from addr=%x, rc(%d)\n", addr, rc);
		return rc;
	}

	reg &= ~mask;
	reg |= val & mask;
	rc = spmi_ext_register_writel(pon->spmi->ctrl, pon->spmi->sid,
							addr, &reg, 1);
	if (rc)
		dev_err(&pon->spmi->dev,
			"Unable to write to addr=%x, rc(%d)\n", addr, rc);
	return rc;
}

/**
 * qpnp_pon_system_pwr_off - Configure system-reset PMIC for shutdown or reset
 * @type: Determines the type of power off to perform - shutdown, reset, etc
 *
 * This function will only configure a single PMIC. The other PMICs in the
 * system are slaved off of it and require no explicit configuration. Once
 * the system-reset PMIC is configured properly, the MSM can drop PS_HOLD to
 * activate the specified configuration.
 */
int qpnp_pon_system_pwr_off(enum pon_power_off_type type)
{
	int rc;
	u8 reg;
	u16 rst_en_reg;
	struct qpnp_pon *pon = sys_reset_dev;

	if (!pon)
		return -ENODEV;

	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
			QPNP_PON_REVISION2(pon->base), &reg, 1);
	if (rc) {
		dev_err(&pon->spmi->dev,
			"Unable to read addr=%x, rc(%d)\n",
			QPNP_PON_REVISION2(pon->base), rc);
		return rc;
	}

	if (reg == PON_REV2_VALUE)
		rst_en_reg = QPNP_PON_PS_HOLD_RST_CTL(pon->base);
	else
		rst_en_reg = QPNP_PON_PS_HOLD_RST_CTL2(pon->base);

	rc = qpnp_pon_masked_write(pon, rst_en_reg, QPNP_PON_RESET_EN, 0);
	if (rc)
		dev_err(&pon->spmi->dev,
			"Unable to write to addr=%x, rc(%d)\n", rst_en_reg, rc);

	/*
	 * We need 10 sleep clock cycles here. But since the clock is
	 * internally generated, we need to add 50% tolerance to be
	 * conservative.
	 */
	udelay(500);

	rc = qpnp_pon_masked_write(pon, QPNP_PON_PS_HOLD_RST_CTL(pon->base),
				   QPNP_PON_POWER_OFF_MASK, type);
	if (rc)
		dev_err(&pon->spmi->dev,
			"Unable to write to addr=%x, rc(%d)\n",
				QPNP_PON_PS_HOLD_RST_CTL(pon->base), rc);

	rc = qpnp_pon_masked_write(pon, rst_en_reg, QPNP_PON_RESET_EN,
						    QPNP_PON_RESET_EN);
	if (rc)
		dev_err(&pon->spmi->dev,
			"Unable to write to addr=%x, rc(%d)\n", rst_en_reg, rc);

	dev_dbg(&pon->spmi->dev, "power off type = 0x%02X\n", type);

	return rc;
}
EXPORT_SYMBOL(qpnp_pon_system_pwr_off);

/**
 * qpnp_pon_is_warm_reset - Checks if the PMIC went through a warm reset.
 *
 * Returns > 0 for warm resets, 0 for not warm reset, < 0 for errors
 *
 * Note that this function will only return the warm vs not-warm reset status
 * of the PMIC that is configured as the system-reset device.
 */
int qpnp_pon_is_warm_reset(void)
{
	struct qpnp_pon *pon = sys_reset_dev;
	int rc;
	u8 reg;

	if (!pon)
		return -EPROBE_DEFER;

	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
			QPNP_PON_WARM_RESET_REASON1(pon->base), &reg, 1);
	if (rc) {
		dev_err(&pon->spmi->dev,
			"Unable to read addr=%x, rc(%d)\n",
			QPNP_PON_WARM_RESET_REASON1(pon->base), rc);
		return rc;
	}

	if (reg)
		return 1;

	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
			QPNP_PON_WARM_RESET_REASON2(pon->base), &reg, 1);
	if (rc) {
		dev_err(&pon->spmi->dev,
			"Unable to read addr=%x, rc(%d)\n",
			QPNP_PON_WARM_RESET_REASON2(pon->base), rc);
		return rc;
	}
	if (reg & QPNP_PON_WARM_RESET_TFT)
		return 1;

	return 0;
}
EXPORT_SYMBOL(qpnp_pon_is_warm_reset);

/**
 * qpnp_pon_wd_config - Disable the wd in a warm reset.
 * @enable: to enable or disable the PON watch dog
 *
 * Returns = 0 for operate successfully, < 0 for errors
 */
int qpnp_pon_wd_config(bool enable)
{
	struct qpnp_pon *pon = sys_reset_dev;
	int rc = 0;

	if (!pon)
		return -EPROBE_DEFER;

	rc = qpnp_pon_masked_write(pon, QPNP_PON_WD_RST_S2_CTL2(pon->base),
			QPNP_PON_WD_EN, enable ? QPNP_PON_WD_EN : 0);
	if (rc)
		dev_err(&pon->spmi->dev,
				"Unable to write to addr=%x, rc(%d)\n",
				QPNP_PON_WD_RST_S2_CTL2(pon->base), rc);

	return rc;
}
EXPORT_SYMBOL(qpnp_pon_wd_config);


/**
 * qpnp_pon_trigger_config - Configures (enable/disable) the PON trigger source
 * @pon_src: PON source to be configured
 * @enable: to enable or disable the PON trigger
 *
 * This function configures the power-on trigger capability of a
 * PON source. If a specific PON trigger is disabled it cannot act
 * as a power-on source to the PMIC.
 */

int qpnp_pon_trigger_config(enum pon_trigger_source pon_src, bool enable)
{
	struct qpnp_pon *pon = sys_reset_dev;
	int rc;

	if (!pon)
		return -EPROBE_DEFER;

	if (pon_src < PON_SMPL || pon_src > PON_KPDPWR_N) {
		dev_err(&pon->spmi->dev, "Invalid PON source\n");
		return -EINVAL;
	}

	rc = qpnp_pon_masked_write(pon, QPNP_PON_TRIGGER_EN(pon->base),
				BIT(pon_src), enable ? BIT(pon_src) : 0);
	if (rc)
		dev_err(&pon->spmi->dev, "Unable to write to addr=%x, rc(%d)\n",
					QPNP_PON_TRIGGER_EN(pon->base), rc);

	return rc;
}
EXPORT_SYMBOL(qpnp_pon_trigger_config);

static struct qpnp_pon_config *
qpnp_get_cfg(struct qpnp_pon *pon, u32 pon_type)
{
	int i;

	for (i = 0; i < pon->num_pon_config; i++) {
		if (pon_type == pon->pon_cfg[i].pon_type)
			return  &pon->pon_cfg[i];
	}

	return NULL;
}

static int
qpnp_pon_input_dispatch(struct qpnp_pon *pon, u32 pon_type)
{
	int rc;
	struct qpnp_pon_config *cfg = NULL;
	u8 pon_rt_sts = 0, pon_rt_bit = 0;

	cfg = qpnp_get_cfg(pon, pon_type);
	if (!cfg)
		return -EINVAL;

	/* Check if key reporting is supported */
	if (!cfg->key_code)
		return 0;

	/* check the RT status to get the current status of the line */
	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
				QPNP_PON_RT_STS(pon->base), &pon_rt_sts, 1);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to read PON RT status\n");
		return rc;
	}

	switch (cfg->pon_type) {
	case PON_KPDPWR:
		pon_rt_bit = QPNP_PON_KPDPWR_N_SET;
		break;
	case PON_RESIN:
		pon_rt_bit = QPNP_PON_RESIN_N_SET;
		break;
	case PON_CBLPWR:
		pon_rt_bit = QPNP_PON_CBLPWR_N_SET;
		break;
	case PON_KPDPWR_RESIN:
		pon_rt_bit = QPNP_PON_KPDPWR_RESIN_BARK_N_SET;
		break;
	default:
		return -EINVAL;
	}

	if (cfg->pon_type == PON_RESIN )
		printk(KERN_INFO "%s: VOLUME DOWN key is %s\n",
				__func__, (pon_rt_sts & pon_rt_bit) ? "pressed" : "released");
	else
		printk(KERN_INFO "%s: PWR key is %s\n",
				__func__, (pon_rt_sts & pon_rt_bit) ? "pressed" : "released");

	input_report_key(pon->pon_input, cfg->key_code,
					(pon_rt_sts & pon_rt_bit));
	input_sync(pon->pon_input);

#ifdef CONFIG_SEC_PATEK_PROJECT
	if((cfg->key_code == KEY_END) && (pon_rt_sts & pon_rt_bit)){
		pon->powerkey_state = 1;
	}else if((cfg->key_code == KEY_END) && !(pon_rt_sts & pon_rt_bit)){
		pon->powerkey_state = 0;
	}
#else
	if((cfg->key_code == 116) && (pon_rt_sts & pon_rt_bit)){
		pon->powerkey_state = 1;
	}else if((cfg->key_code == 116) && !(pon_rt_sts & pon_rt_bit)){
		pon->powerkey_state = 0;
	}
#endif	

#if defined(CONFIG_SEC_PM)
	/* RESIN is used for VOL DOWN key, it should report the keycode for kernel panic */
	if((cfg->key_code == 114) && (pon_rt_sts & pon_rt_bit)){
		pon->powerkey_state = 1;
	}else if((cfg->key_code == 114) && !(pon_rt_sts & pon_rt_bit)){
		pon->powerkey_state = 0;
	}
#endif

#ifdef CONFIG_SEC_DEBUG
#ifdef CONFIG_SEC_PATEK_PROJECT
	sec_debug_check_crash_key(116, pon->powerkey_state);
#else
	sec_debug_check_crash_key(cfg->key_code, pon->powerkey_state);
#endif
#endif
#if defined(CONFIG_SEC_PATEK_PROJECT) || defined(CONFIG_SEC_S_PROJECT)
	check_pkey_press=pon->powerkey_state;
#endif
	return 0;
}

#if defined(CONFIG_SEC_PATEK_PROJECT) || defined(CONFIG_SEC_S_PROJECT)
int check_short_pkey(void)
{
	return check_pkey_press;
}
EXPORT_SYMBOL(check_short_pkey);
#endif

static irqreturn_t qpnp_kpdpwr_irq(int irq, void *_pon)
{
	int rc;
	struct qpnp_pon *pon = _pon;

	rc = qpnp_pon_input_dispatch(pon, PON_KPDPWR);
	if (rc)
		dev_err(&pon->spmi->dev, "Unable to send input event\n");

	return IRQ_HANDLED;
}

static irqreturn_t qpnp_kpdpwr_bark_irq(int irq, void *_pon)
{
	return IRQ_HANDLED;
}

static irqreturn_t qpnp_resin_irq(int irq, void *_pon)
{
	int rc;
	struct qpnp_pon *pon = _pon;

	rc = qpnp_pon_input_dispatch(pon, PON_RESIN);
	if (rc)
		dev_err(&pon->spmi->dev, "Unable to send input event\n");
	return IRQ_HANDLED;
}

static irqreturn_t qpnp_kpdpwr_resin_bark_irq(int irq, void *_pon)
{
	return IRQ_HANDLED;
}

static irqreturn_t qpnp_cblpwr_irq(int irq, void *_pon)
{
	int rc;
	struct qpnp_pon *pon = _pon;

	rc = qpnp_pon_input_dispatch(pon, PON_CBLPWR);
	if (rc)
		dev_err(&pon->spmi->dev, "Unable to send input event\n");

	return IRQ_HANDLED;
}

static void bark_work_func(struct work_struct *work)
{
	int rc;
	u8 pon_rt_sts = 0;
	struct qpnp_pon_config *cfg;
	struct qpnp_pon *pon =
		container_of(work, struct qpnp_pon, bark_work.work);

	cfg = qpnp_get_cfg(pon, PON_RESIN);
	if (!cfg) {
		dev_err(&pon->spmi->dev, "Invalid config pointer\n");
		goto err_return;
	}

	/* enable reset */
	rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
				QPNP_PON_S2_CNTL_EN, QPNP_PON_S2_CNTL_EN);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 enable\n");
		goto err_return;
	}
	/* bark RT status update delay */
	msleep(100);
	/* read the bark RT status */
	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
				QPNP_PON_RT_STS(pon->base), &pon_rt_sts, 1);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to read PON RT status\n");
		goto err_return;
	}

	if (!(pon_rt_sts & QPNP_PON_RESIN_BARK_N_SET)) {
		/* report the key event and enable the bark IRQ */
		input_report_key(pon->pon_input, cfg->key_code, 0);
		input_sync(pon->pon_input);
		enable_irq(cfg->bark_irq);
	} else {
		/* disable reset */
		rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
				QPNP_PON_S2_CNTL_EN, 0);
		if (rc) {
			dev_err(&pon->spmi->dev,
				"Unable to configure S2 enable\n");
			goto err_return;
		}
		/* re-arm the work */
		schedule_delayed_work(&pon->bark_work, QPNP_KEY_STATUS_DELAY);
	}

err_return:
	return;
}

static irqreturn_t qpnp_resin_bark_irq(int irq, void *_pon)
{
	int rc;
	struct qpnp_pon *pon = _pon;
	struct qpnp_pon_config *cfg;

	/* disable the bark interrupt */
	disable_irq_nosync(irq);

	cfg = qpnp_get_cfg(pon, PON_RESIN);
	if (!cfg) {
		dev_err(&pon->spmi->dev, "Invalid config pointer\n");
		goto err_exit;
	}

	/* disable reset */
	rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
					QPNP_PON_S2_CNTL_EN, 0);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 enable\n");
		goto err_exit;
	}

	/* report the key event */
	input_report_key(pon->pon_input, cfg->key_code, 1);
	input_sync(pon->pon_input);
	/* schedule work to check the bark status for key-release */
	schedule_delayed_work(&pon->bark_work, QPNP_KEY_STATUS_DELAY);
err_exit:
	return IRQ_HANDLED;
}

static int __devinit
qpnp_config_pull(struct qpnp_pon *pon, struct qpnp_pon_config *cfg)
{
	int rc;
	u8 pull_bit;

#if defined(CONFIG_SEC_K_PROJECT) || \
	defined(CONFIG_SEC_KACTIVE_PROJECT) || defined(CONFIG_SEC_KSPORTS_PROJECT) || \
	defined(CONFIG_SEC_S_PROJECT) || defined(CONFIG_SEC_PATEK_PROJECT)
	/* Do nothing in case of KPDPWR_RESIN*/
	if (cfg->pon_type == PON_KPDPWR_RESIN)
		return 0;
#endif

	switch (cfg->pon_type) {
	case PON_KPDPWR:
		pull_bit = QPNP_PON_KPDPWR_PULL_UP;
		break;
	case PON_RESIN:
		pull_bit = QPNP_PON_RESIN_PULL_UP;
		break;
	case PON_CBLPWR:
		pull_bit = QPNP_PON_CBLPWR_PULL_UP;
		break;
	case PON_KPDPWR_RESIN:
		pull_bit = QPNP_PON_KPDPWR_PULL_UP | QPNP_PON_RESIN_PULL_UP;
		break;
	default:
		return -EINVAL;
	}

	rc = qpnp_pon_masked_write(pon, QPNP_PON_PULL_CTL(pon->base),
				pull_bit, cfg->pull_up ? pull_bit : 0);
	if (rc)
		dev_err(&pon->spmi->dev, "Unable to config pull-up\n");

	return rc;
}

static int __devinit
qpnp_config_reset(struct qpnp_pon *pon, struct qpnp_pon_config *cfg)
{
	int rc;
	u8 i;
	u16 s1_timer_addr, s2_timer_addr;

	switch (cfg->pon_type) {
	case PON_KPDPWR:
		s1_timer_addr = QPNP_PON_KPDPWR_S1_TIMER(pon->base);
		s2_timer_addr = QPNP_PON_KPDPWR_S2_TIMER(pon->base);
		break;
	case PON_RESIN:
		s1_timer_addr = QPNP_PON_RESIN_S1_TIMER(pon->base);
		s2_timer_addr = QPNP_PON_RESIN_S2_TIMER(pon->base);
		break;
	case PON_KPDPWR_RESIN:
		s1_timer_addr = QPNP_PON_KPDPWR_RESIN_S1_TIMER(pon->base);
		s2_timer_addr = QPNP_PON_KPDPWR_RESIN_S2_TIMER(pon->base);
		break;
	default:
		return -EINVAL;
	}
	/* disable S2 reset */
	rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
				QPNP_PON_S2_CNTL_EN, 0);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 enable\n");
		return rc;
	}

	usleep(100);

	/* configure s1 timer, s2 timer and reset type */
	for (i = 0; i < PON_S1_COUNT_MAX + 1; i++) {
		if (cfg->s1_timer <= s1_delay[i])
			break;
	}
	rc = qpnp_pon_masked_write(pon, s1_timer_addr,
				QPNP_PON_S1_TIMER_MASK, i);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S1 timer\n");
		return rc;
	}

	i = 0;
	if (cfg->s2_timer) {
		i = cfg->s2_timer / 10;
		i = ilog2(i + 1);
	}

	rc = qpnp_pon_masked_write(pon, s2_timer_addr,
				QPNP_PON_S2_TIMER_MASK, i);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 timer\n");
		return rc;
	}

#ifdef CONFIG_ARCH_MSM8226
#ifdef CONFIG_SEC_DEBUG
	/* Configure reset type:
	 * Debug level MID/HIGH: WARM Reset
	 * Debug level LOW: HARD Reset
	 */
	if (sec_debug_is_enabled()) {
		cfg->s2_type = 1;
	} else {
	/* For Millet VZW and Mattise VZW models always do warm reset */
#if defined(CONFIG_MACH_MATISSELTE_VZW) || defined(CONFIG_MACH_MILLETLTE_VZW)
		cfg->s2_type = 1;
#elif defined(CONFIG_SEC_MILLET_PROJECT) || defined(CONFIG_MACH_MEGA23GEUR_OPEN) || defined(CONFIG_MACH_MEGA2LTE_KTT)
		cfg->s2_type = 8;  //dVDD hard reset
#elif defined(CONFIG_MACH_MATISSE3G_CHN_OPEN) || defined(CONFIG_MACH_MILLET3G_CHN_OPEN)
		cfg->s2_type = 8;  //dVDD reset
#else
		cfg->s2_type = 7;
#endif
	}
#endif
#endif

#ifdef CONFIG_SEC_DEBUG
#if defined(CONFIG_MACH_KANAS3G_CTC) || defined(CONFIG_MACH_CHAGALL_LTE) || defined(CONFIG_MACH_KLIMT_LTE)
	if (sec_debug_is_enabled()) {
		cfg->s2_type = 1;	// warm reset
	} else {
		cfg->s2_type = 8;	// dVdd hard reset
	}
#endif
#endif

	rc = qpnp_pon_masked_write(pon, cfg->s2_cntl_addr,
				QPNP_PON_S2_CNTL_TYPE_MASK, (u8)cfg->s2_type);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 reset type\n");
		return rc;
	}

	/* enable S2 reset */
	rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
				QPNP_PON_S2_CNTL_EN, QPNP_PON_S2_CNTL_EN);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 enable\n");
		return rc;
	}

	return 0;
}

#ifdef CONFIG_SEC_PM
static int
qpnp_control_s2_reset(struct qpnp_pon *pon, struct qpnp_pon_config *cfg, int on)
{
	int rc;

	/* control S2 reset */
	rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
				QPNP_PON_S2_CNTL_EN, on? QPNP_PON_S2_CNTL_EN : 0);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to configure S2 enable\n");
		return rc;
	}

	return 0;
}
#endif

static int __devinit
qpnp_pon_request_irqs(struct qpnp_pon *pon, struct qpnp_pon_config *cfg)
{
	int rc = 0;

	switch (cfg->pon_type) {
	case PON_KPDPWR:
#ifdef CONFIG_SEC_DEBUG
		if (sec_debug_is_enabled()) {
			rc = qpnp_pon_input_dispatch(pon, PON_KPDPWR);
			if (rc)
				dev_err(&pon->spmi->dev, "Fail to first check to send input event\n");
		}
#endif
		rc = devm_request_irq(&pon->spmi->dev, cfg->state_irq,
							qpnp_kpdpwr_irq,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						"qpnp_kpdpwr_status", pon);
		if (rc < 0) {
			dev_err(&pon->spmi->dev, "Can't request %d IRQ\n",
							cfg->state_irq);
			return rc;
		}
		if (cfg->use_bark) {
			rc = devm_request_irq(&pon->spmi->dev, cfg->bark_irq,
						qpnp_kpdpwr_bark_irq,
						IRQF_TRIGGER_RISING,
						"qpnp_kpdpwr_bark", pon);
			if (rc < 0) {
				dev_err(&pon->spmi->dev,
					"Can't request %d IRQ\n",
						cfg->bark_irq);
				return rc;
			}
		}
		break;
	case PON_RESIN:
		rc = devm_request_irq(&pon->spmi->dev, cfg->state_irq,
							qpnp_resin_irq,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
						"qpnp_resin_status", pon);
		if (rc < 0) {
			dev_err(&pon->spmi->dev, "Can't request %d IRQ\n",
							cfg->state_irq);
			return rc;
		}
		if (cfg->use_bark) {
			rc = devm_request_irq(&pon->spmi->dev, cfg->bark_irq,
						qpnp_resin_bark_irq,
						IRQF_TRIGGER_RISING,
						"qpnp_resin_bark", pon);
			if (rc < 0) {
				dev_err(&pon->spmi->dev,
					"Can't request %d IRQ\n",
						cfg->bark_irq);
				return rc;
			}
		}
		break;
	case PON_CBLPWR:
		rc = devm_request_irq(&pon->spmi->dev, cfg->state_irq,
							qpnp_cblpwr_irq,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
					"qpnp_cblpwr_status", pon);
		if (rc < 0) {
			dev_err(&pon->spmi->dev, "Can't request %d IRQ\n",
							cfg->state_irq);
			return rc;
		}
		break;
	case PON_KPDPWR_RESIN:
		if (cfg->use_bark) {
			rc = devm_request_irq(&pon->spmi->dev, cfg->bark_irq,
					qpnp_kpdpwr_resin_bark_irq,
					IRQF_TRIGGER_RISING,
					"qpnp_kpdpwr_resin_bark", pon);
			if (rc < 0) {
				dev_err(&pon->spmi->dev,
					"Can't request %d IRQ\n",
						cfg->bark_irq);
				return rc;
			}
		}
		break;
	default:
		return -EINVAL;
	}

	/* mark the interrupts wakeable if they support linux-key */
	if (cfg->key_code) {
		enable_irq_wake(cfg->state_irq);
#ifdef CONFIG_SEC_PM_DEBUG
		wake_enabled = true;
#endif

		/* special handling for RESIN due to a hardware bug */
		if (cfg->pon_type == PON_RESIN && cfg->support_reset)
			enable_irq_wake(cfg->bark_irq);
	}

	return rc;
}

static int __devinit
qpnp_pon_config_input(struct qpnp_pon *pon,  struct qpnp_pon_config *cfg)
{
	if (!pon->pon_input) {
		pon->pon_input = input_allocate_device();
		if (!pon->pon_input) {
			dev_err(&pon->spmi->dev,
				"Can't allocate pon input device\n");
			return -ENOMEM;
		}
		pon->pon_input->name = "qpnp_pon";
		pon->pon_input->phys = "qpnp_pon/input0";
	}

	/* don't send dummy release event when system resumes */
	__set_bit(INPUT_PROP_NO_DUMMY_RELEASE, pon->pon_input->propbit);
	input_set_capability(pon->pon_input, EV_KEY, cfg->key_code);

	return 0;
}

static int __devinit qpnp_pon_config_init(struct qpnp_pon *pon)
{
	int rc = 0, i = 0;
	struct device_node *pp = NULL;
	struct qpnp_pon_config *cfg;
	u8 pon_ver;
	u8 pmic_type;
	u8 revid_rev4;

	/* Check if it is rev B */
	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
			QPNP_PON_REVISION2(pon->base), &pon_ver, 1);
	if (rc) {
		dev_err(&pon->spmi->dev,
			"Unable to read addr=%x, rc(%d)\n",
			QPNP_PON_REVISION2(pon->base), rc);
		return rc;
	}

	/* iterate through the list of pon configs */
	while ((pp = of_get_next_child(pon->spmi->dev.of_node, pp))) {

		cfg = &pon->pon_cfg[i++];

		rc = of_property_read_u32(pp, "qcom,pon-type", &cfg->pon_type);
		if (rc) {
			dev_err(&pon->spmi->dev, "PON type not specified\n");
			return rc;
		}

		switch (cfg->pon_type) {
		case PON_KPDPWR:
			cfg->state_irq = spmi_get_irq_byname(pon->spmi,
							NULL, "kpdpwr");
			if (cfg->state_irq < 0) {
				dev_err(&pon->spmi->dev,
					"Unable to get kpdpwr irq\n");
				return cfg->state_irq;
			}

			rc = of_property_read_u32(pp, "qcom,support-reset",
							&cfg->support_reset);
			if (rc && rc != -EINVAL) {
				dev_err(&pon->spmi->dev,
					"Unable to read 'support-reset'\n");
				return rc;
			}

#ifdef CONFIG_SEC_PM
			rc = of_property_read_u32(pp, "qcom,disable-reset",
							&cfg->disable_reset);
			if (rc && rc != -EINVAL) {
				dev_err(&pon->spmi->dev,
					"Unable to read 'disable-reset'\n");
				return rc;
			}
#endif

			cfg->use_bark = of_property_read_bool(pp,
							"qcom,use-bark");
			if (cfg->use_bark) {
				cfg->bark_irq = spmi_get_irq_byname(pon->spmi,
							NULL, "kpdpwr-bark");
				if (cfg->bark_irq < 0) {
					dev_err(&pon->spmi->dev,
					"Unable to get kpdpwr-bark irq\n");
					return cfg->bark_irq;
				}
			}

			/* If the value read from REVISION2 register is 0x00,
			 * then there is a single register to control s2 reset.
			 * Otherwise there are separate registers for s2 reset
			 * type and s2 reset enable
			 */
			if (pon_ver == PON_REV2_VALUE) {
				cfg->s2_cntl_addr = cfg->s2_cntl2_addr =
					QPNP_PON_KPDPWR_S2_CNTL(pon->base);
			} else {
				cfg->s2_cntl_addr =
					QPNP_PON_KPDPWR_S2_CNTL(pon->base);
				cfg->s2_cntl2_addr =
					QPNP_PON_KPDPWR_S2_CNTL2(pon->base);
			}

			break;
		case PON_RESIN:
			cfg->state_irq = spmi_get_irq_byname(pon->spmi,
							NULL, "resin");
			if (cfg->state_irq < 0) {
				dev_err(&pon->spmi->dev,
					"Unable to get resin irq\n");
				return cfg->bark_irq;
			}

			rc = of_property_read_u32(pp, "qcom,support-reset",
							&cfg->support_reset);
			if (rc && rc != -EINVAL) {
				dev_err(&pon->spmi->dev,
					"Unable to read 'support-reset'\n");
				return rc;
			}

#ifdef CONFIG_SEC_PM
			rc = of_property_read_u32(pp, "qcom,disable-reset",
							&cfg->disable_reset);
			if (rc && rc != -EINVAL) {
				dev_err(&pon->spmi->dev,
					"Unable to read 'disable-reset'\n");
				return rc;
			}
#endif

			cfg->use_bark = of_property_read_bool(pp,
							"qcom,use-bark");

			rc = spmi_ext_register_readl(pon->spmi->ctrl,
					pon->spmi->sid, PMIC_VERSION_REG,
						&pmic_type, 1);

			if (rc) {
				dev_err(&pon->spmi->dev,
				"Unable to read PMIC type\n");
				return rc;
			}

			if (pmic_type == PMIC_VER_8941) {

				rc = spmi_ext_register_readl(pon->spmi->ctrl,
					pon->spmi->sid, PMIC_VERSION_REV4_REG,
							&revid_rev4, 1);

				if (rc) {
					dev_err(&pon->spmi->dev,
					"Unable to read PMIC revision ID\n");
					return rc;
				}

				/*PM8941 V3 does not have harware bug. Hence
				bark is not required from PMIC versions 3.0*/
				if (!(revid_rev4 == PMIC8941_V1_REV4 ||
					revid_rev4 == PMIC8941_V2_REV4)) {
					cfg->support_reset = false;
					cfg->use_bark = false;
				}
			}

			if (cfg->use_bark) {
				cfg->bark_irq = spmi_get_irq_byname(pon->spmi,
							NULL, "resin-bark");
				if (cfg->bark_irq < 0) {
					dev_err(&pon->spmi->dev,
					"Unable to get resin-bark irq\n");
					return cfg->bark_irq;
				}
			}

			if (pon_ver == PON_REV2_VALUE) {
				cfg->s2_cntl_addr = cfg->s2_cntl2_addr =
					QPNP_PON_RESIN_S2_CNTL(pon->base);
			} else {
				cfg->s2_cntl_addr =
					QPNP_PON_RESIN_S2_CNTL(pon->base);
				cfg->s2_cntl2_addr =
					QPNP_PON_RESIN_S2_CNTL2(pon->base);
			}

			break;
		case PON_CBLPWR:
			cfg->state_irq = spmi_get_irq_byname(pon->spmi,
							NULL, "cblpwr");
			if (cfg->state_irq < 0) {
				dev_err(&pon->spmi->dev,
						"Unable to get cblpwr irq\n");
				return rc;
			}
			break;
		case PON_KPDPWR_RESIN:
			rc = of_property_read_u32(pp, "qcom,support-reset",
							&cfg->support_reset);
			if (rc && rc != -EINVAL) {
				dev_err(&pon->spmi->dev,
					"Unable to read 'support-reset'\n");
				return rc;
			}

#ifdef CONFIG_SEC_PM
			rc = of_property_read_u32(pp, "qcom,disable-reset",
							&cfg->disable_reset);
			if (rc && rc != -EINVAL) {
				dev_err(&pon->spmi->dev,
					"Unable to read 'disable-reset'\n");
				return rc;
			}
#endif

			cfg->use_bark = of_property_read_bool(pp,
							"qcom,use-bark");
			if (cfg->use_bark) {
				cfg->bark_irq = spmi_get_irq_byname(pon->spmi,
						NULL, "kpdpwr-resin-bark");
				if (cfg->bark_irq < 0) {
					dev_err(&pon->spmi->dev,
					"Unable to get kpdpwr-resin-bark irq\n");
					return cfg->bark_irq;
				}
			}

			if (pon_ver == PON_REV2_VALUE) {
				cfg->s2_cntl_addr = cfg->s2_cntl2_addr =
				QPNP_PON_KPDPWR_RESIN_S2_CNTL(pon->base);
			} else {
				cfg->s2_cntl_addr =
				QPNP_PON_KPDPWR_RESIN_S2_CNTL(pon->base);
				cfg->s2_cntl2_addr =
				QPNP_PON_KPDPWR_RESIN_S2_CNTL2(pon->base);
			}

			break;
		default:
			dev_err(&pon->spmi->dev, "PON RESET %d not supported",
								cfg->pon_type);
			return -EINVAL;
		}

		if (cfg->support_reset) {
			/*
			 * Get the reset parameters (bark debounce time and
			 * reset debounce time) for the reset line.
			 */
#if defined(CONFIG_MACH_KLTE_VZW) || defined(CONFIG_MACH_CHAGALL_VZW)
			rc = of_property_read_u32(pp, "qcom,s1-timer2",
							&cfg->s1_timer);
#else
			rc = of_property_read_u32(pp, "qcom,s1-timer",
							&cfg->s1_timer);
#endif
			if (rc) {
				dev_err(&pon->spmi->dev,
					"Unable to read s1-timer\n");
				return rc;
			}
			if (cfg->s1_timer > QPNP_PON_S1_TIMER_MAX) {
				dev_err(&pon->spmi->dev,
					"Incorrect S1 debounce time\n");
				return -EINVAL;
			}
#if defined(CONFIG_MACH_KLTE_VZW) || defined(CONFIG_MACH_CHAGALL_VZW)
			rc = of_property_read_u32(pp, "qcom,s2-timer2",
							&cfg->s2_timer);
#else
			rc = of_property_read_u32(pp, "qcom,s2-timer",
							&cfg->s2_timer);
#endif
			if (rc) {
				dev_err(&pon->spmi->dev,
					"Unable to read s2-timer\n");
				return rc;
			}
			if (cfg->s2_timer > QPNP_PON_S2_TIMER_MAX) {
				dev_err(&pon->spmi->dev,
					"Incorrect S2 debounce time\n");
				return -EINVAL;
			}
			rc = of_property_read_u32(pp, "qcom,s2-type",
							&cfg->s2_type);
			if (rc) {
				dev_err(&pon->spmi->dev,
					"Unable to read s2-type\n");
				return rc;
			}
			if (cfg->s2_type > QPNP_PON_RESET_TYPE_MAX) {
				dev_err(&pon->spmi->dev,
					"Incorrect reset type specified\n");
				return -EINVAL;
			}

		}
		/*
		 * Get the standard-key parameters. This might not be
		 * specified if there is no key mapping on the reset line.
		 */
		rc = of_property_read_u32(pp, "linux,code", &cfg->key_code);

#ifdef CONFIG_SEC_PATEK_PROJECT
		if(cfg->key_code == 116){
			dev_err(&pon->spmi->dev,
				"patek power key code changed to %d(%d)\n",
				KEY_END, cfg->key_code);
			cfg->key_code = KEY_END;
		}
#else
		if (rc && rc != -EINVAL) {
			dev_err(&pon->spmi->dev,
				"Unable to read key-code\n");
			return rc;
		}
#endif

		/* Register key configuration */
		if (cfg->key_code) {
			rc = qpnp_pon_config_input(pon, cfg);
			if (rc < 0)
				return rc;
		}
		/* get the pull-up configuration */
		rc = of_property_read_u32(pp, "qcom,pull-up", &cfg->pull_up);
		if (rc && rc != -EINVAL) {
			dev_err(&pon->spmi->dev, "Unable to read pull-up\n");
			return rc;
		}
	}

	/* register the input device */
	if (pon->pon_input) {
		rc = input_register_device(pon->pon_input);
		if (rc) {
			dev_err(&pon->spmi->dev,
				"Can't register pon key: %d\n", rc);
			goto free_input_dev;
		}
	}

	for (i = 0; i < pon->num_pon_config; i++) {
		cfg = &pon->pon_cfg[i];
		/* Configure the pull-up */
		rc = qpnp_config_pull(pon, cfg);
		if (rc) {
			dev_err(&pon->spmi->dev, "Unable to config pull-up\n");
			goto unreg_input_dev;
		}
		/* Configure the reset-configuration */
		if (cfg->support_reset) {
			rc = qpnp_config_reset(pon, cfg);
			if (rc) {
				dev_err(&pon->spmi->dev,
					"Unable to config pon reset\n");
				goto unreg_input_dev;
			}
		} else {
			/* disable S2 reset */
			rc = qpnp_pon_masked_write(pon, cfg->s2_cntl2_addr,
						QPNP_PON_S2_CNTL_EN, 0);
			if (rc) {
				dev_err(&pon->spmi->dev,
					"Unable to disable S2 reset\n");
				goto unreg_input_dev;
			}
		}

#ifdef CONFIG_SEC_PM
		if (cfg->disable_reset)
			qpnp_control_s2_reset(pon, cfg, !cfg->disable_reset);
#endif

#ifdef CONFIG_MACH_HLTEVZW
		/* Disable power key reset on HLTE VZW with debug low by request */
		if (cfg->pon_type == PON_KPDPWR) {
			if (0 == sec_debug_is_enabled())
				qpnp_control_s2_reset(pon, cfg, 0);
		}
#endif

		rc = qpnp_pon_request_irqs(pon, cfg);
		if (rc) {
			dev_err(&pon->spmi->dev, "Unable to request-irq's\n");
			goto unreg_input_dev;
		}
	}

	device_init_wakeup(&pon->spmi->dev, 1);

	return rc;

unreg_input_dev:
	if (pon->pon_input)
		input_unregister_device(pon->pon_input);
free_input_dev:
	if (pon->pon_input)
		input_free_device(pon->pon_input);
	return rc;
}

static ssize_t  sysfs_powerkey_onoff_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct qpnp_pon *pon = dev_get_drvdata(dev);
	printk(KERN_INFO "%s\n",__func__);
	if (pon->powerkey_state == 1) {
		printk(KERN_INFO "powerkey is pressed\n");
		return snprintf(buf, 5, "%d\n", pon->powerkey_state);
	} else {
		printk(KERN_INFO "powerkey is released\n");
		return snprintf(buf, 5, "%d\n", pon->powerkey_state);
	}
}
static DEVICE_ATTR(sec_powerkey_pressed, 0664 , sysfs_powerkey_onoff_show, NULL);

#ifdef CONFIG_SEC_PM_DEBUG
static int qpnp_wake_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	struct qpnp_pon_config *cfg;

	ret = param_set_bool(val, kp);
	if (ret) {
		pr_err("Unable to set qpnp_wake_enabled: %d\n", ret);
		return ret;
	}

	cfg = qpnp_get_cfg(sys_reset_dev, PON_KPDPWR);
	if (!cfg) {
		pr_err("Invalid config pointer\n");
		return -EFAULT;
	}

	if (!wake_enabled)
		disable_irq_wake(cfg->state_irq);
	else
		enable_irq_wake(cfg->state_irq);

	pr_info("%s: wake_enabled = %d\n", KBUILD_MODNAME, wake_enabled);

	return ret;
}

static struct kernel_param_ops module_ops = {
	.set = qpnp_wake_enabled,
	.get = param_get_bool,
};

module_param_cb(wake_enabled, &module_ops, &wake_enabled, 0644);

static int qpnp_reset_enabled(const char *val, const struct kernel_param *kp)
{
	int ret = 0;
	struct qpnp_pon_config *cfg;

	ret = param_set_bool(val, kp);
	if (ret) {
		pr_err("Unable to set qpnp_reset_enabled: %d\n", ret);
		return ret;
	}

	cfg = qpnp_get_cfg(sys_reset_dev, PON_KPDPWR);
	if (!cfg) {
		pr_err("Invalid config pointer\n");
		return -EFAULT;
	}

	if (!reset_enabled)
		qpnp_control_s2_reset(sys_reset_dev, cfg, 0);
	else
		qpnp_control_s2_reset(sys_reset_dev, cfg, 1);

	pr_info("%s: reset_enabled = %d\n", KBUILD_MODNAME, reset_enabled);

	return ret;
}

static struct kernel_param_ops reset_module_ops = {
	.set = qpnp_reset_enabled,
	.get = param_get_bool,
};

module_param_cb(reset_enabled, &reset_module_ops, &reset_enabled, 0644);
#endif

#ifdef CONFIG_SEC_PM
int qpnp_pon_set_wd_timer(u8 s1_timer, u8 s2_timer, u8 reset_type)
{
	struct qpnp_pon *pon = sys_reset_dev;
	u8 data;

	spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid, 0x854, &data, 1);
	pr_debug("%s: 0x854=0x%x\n", __func__, data);
	spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid, 0x855, &data, 1);
	pr_debug("%s: 0x855=0x%x\n", __func__, data);
	spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid, 0x856, &data, 1);
	pr_debug("%s: 0x856=0x%x\n", __func__, data);

	data = s1_timer; /* S1_TIMER */
	spmi_ext_register_writel(pon->spmi->ctrl, pon->spmi->sid, 0x854, &data, 1);
	data = s2_timer; /* S2_TIMER */
	spmi_ext_register_writel(pon->spmi->ctrl, pon->spmi->sid, 0x855, &data, 1);
	data = reset_type; /* RESET_TYPE */
	spmi_ext_register_writel(pon->spmi->ctrl, pon->spmi->sid, 0x856, &data, 1);
	data = 0x1; /* WD_RESET_PET to clear WD timer */
	spmi_ext_register_writel(pon->spmi->ctrl, pon->spmi->sid, 0x858, &data, 1);

	spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid, 0x854, &data, 1);
	pr_info("%s: 0x854=0x%x\n", __func__, data);
	spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid, 0x855, &data, 1);
	pr_info("%s: 0x855=0x%x\n", __func__, data);
	spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid, 0x856, &data, 1);
	pr_info("%s: 0x856=0x%x\n", __func__, data);

	return 0;
}
EXPORT_SYMBOL(qpnp_pon_set_wd_timer);
#endif

static int __devinit qpnp_pon_probe(struct spmi_device *spmi)
{
	struct qpnp_pon *pon;
	struct resource *pon_resource;
	struct device_node *itr = NULL;
	u32 delay = 0, s3_debounce = 0;
	int rc, sys_reset, index;
	u8 pon_sts = 0, buf[2];
	const char *s3_src;
	u8 s3_src_reg;
	u16 poff_sts = 0;
	struct device *sec_powerkey;
	int ret;

	pon = devm_kzalloc(&spmi->dev, sizeof(struct qpnp_pon),
							GFP_KERNEL);
	if (!pon) {
		dev_err(&spmi->dev, "Can't allocate qpnp_pon\n");
		return -ENOMEM;
	}

	sys_reset = of_property_read_bool(spmi->dev.of_node,
						"qcom,system-reset");
	if (sys_reset && sys_reset_dev) {
		dev_err(&spmi->dev, "qcom,system-reset property can only be specified for one device on the system\n");
		return -EINVAL;
	} else if (sys_reset) {
		sys_reset_dev = pon;
	}

	pon->spmi = spmi;

	/* get the total number of pon configurations */
	while ((itr = of_get_next_child(spmi->dev.of_node, itr)))
		pon->num_pon_config++;

	if (!pon->num_pon_config) {
		/* No PON config., do not register the driver */
		dev_err(&spmi->dev, "No PON config. specified\n");
		return -EINVAL;
	}

	pon->pon_cfg = devm_kzalloc(&spmi->dev,
			sizeof(struct qpnp_pon_config) * pon->num_pon_config,
								GFP_KERNEL);

	pon_resource = spmi_get_resource(spmi, NULL, IORESOURCE_MEM, 0);
	if (!pon_resource) {
		dev_err(&spmi->dev, "Unable to get PON base address\n");
		return -ENXIO;
	}
	pon->base = pon_resource->start;

	/* PON reason */
	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
				QPNP_PON_REASON1(pon->base), &pon_sts, 1);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to read PON_RESASON1 reg\n");
		return rc;
	}

	boot_reason = ffs(pon_sts);
	index = ffs(pon_sts) - 1;
	cold_boot = !qpnp_pon_is_warm_reset();
	if (index >= ARRAY_SIZE(qpnp_pon_reason) || index < 0)
		dev_info(&pon->spmi->dev,
			"PMIC@SID%d Power-on reason: Unknown and '%s' boot\n",
			pon->spmi->sid, cold_boot ? "cold" : "warm");
	else
		dev_info(&pon->spmi->dev,
			"PMIC@SID%d Power-on reason: %s and '%s' boot\n",
			pon->spmi->sid, qpnp_pon_reason[index],
			cold_boot ? "cold" : "warm");

	/* POFF reason */
	rc = spmi_ext_register_readl(pon->spmi->ctrl, pon->spmi->sid,
				QPNP_POFF_REASON1(pon->base),
				buf, 2);
	if (rc) {
		dev_err(&pon->spmi->dev, "Unable to read POFF_RESASON regs\n");
		return rc;
	}
	poff_sts = buf[0] | (buf[1] << 8);
	index = ffs(poff_sts) - 1;
	if (index >= ARRAY_SIZE(qpnp_poff_reason) || index < 0)
		dev_info(&pon->spmi->dev,
				"PMIC@SID%d: Unknown power-off reason\n",
				pon->spmi->sid);
	else
		dev_info(&pon->spmi->dev,
				"PMIC@SID%d: Power-off reason: %s\n",
				pon->spmi->sid,
				qpnp_poff_reason[index]);

	rc = of_property_read_u32(pon->spmi->dev.of_node,
				"qcom,pon-dbc-delay", &delay);
	if (rc) {
		if (rc != -EINVAL) {
			dev_err(&spmi->dev, "Unable to read debounce delay\n");
			return rc;
		}
	} else {
		delay = (delay << QPNP_PON_DELAY_BIT_SHIFT) / USEC_PER_SEC;
		delay = ilog2(delay);
		rc = qpnp_pon_masked_write(pon, QPNP_PON_DBC_CTL(pon->base),
						QPNP_PON_DBC_DELAY_MASK, delay);
		if (rc) {
			dev_err(&spmi->dev, "Unable to set PON debounce\n");
			return rc;
		}
	}

	/* program s3 debounce */
	rc = of_property_read_u32(pon->spmi->dev.of_node,
				"qcom,s3-debounce", &s3_debounce);
	if (rc) {
		if (rc != -EINVAL) {
			dev_err(&pon->spmi->dev, "Unable to read s3 timer\n");
			return rc;
		}
	} else {
		if (s3_debounce > QPNP_PON_S3_TIMER_SECS_MAX) {
			dev_info(&pon->spmi->dev,
				"Exceeded S3 max value, set it to max\n");
			s3_debounce = QPNP_PON_S3_TIMER_SECS_MAX;
		}

		/* 0 is a special value to indicate instant s3 reset */
		if (s3_debounce != 0)
			s3_debounce = ilog2(s3_debounce);
		rc = qpnp_pon_masked_write(pon, QPNP_PON_S3_DBC_CTL(pon->base),
				QPNP_PON_S3_DBC_DELAY_MASK, s3_debounce);
		if (rc) {
			dev_err(&spmi->dev, "Unable to set S3 debounce\n");
			return rc;
		}
	}

	/* program s3 source */
	s3_src = "kpdpwr-and-resin";
	rc = of_property_read_string(pon->spmi->dev.of_node,
				"qcom,s3-src", &s3_src);
	if (rc && rc != -EINVAL) {
		dev_err(&pon->spmi->dev, "Unable to read s3 timer\n");
		return rc;
	}

	if (!strcmp(s3_src, "kpdpwr"))
		s3_src_reg = QPNP_PON_S3_SRC_KPDPWR;
	else if (!strcmp(s3_src, "resin"))
		s3_src_reg = QPNP_PON_S3_SRC_RESIN;
	else if (!strcmp(s3_src, "kpdpwr-or-resin"))
		s3_src_reg = QPNP_PON_S3_SRC_KPDPWR_OR_RESIN;
	else /* default combination */
		s3_src_reg = QPNP_PON_S3_SRC_KPDPWR_AND_RESIN;

	/* S3 source is a write once register. If the register has
	 * been configured by bootloader then this operation will
	 * not be effective. */
	rc = qpnp_pon_masked_write(pon, QPNP_PON_S3_SRC(pon->base),
			QPNP_PON_S3_SRC_MASK, s3_src_reg);
	if (rc) {
		dev_err(&spmi->dev,
			"Unable to program s3 source\n");
		return rc;
	}

	dev_set_drvdata(&spmi->dev, pon);

	INIT_DELAYED_WORK(&pon->bark_work, bark_work_func);

	/* register the PON configurations */
	rc = qpnp_pon_config_init(pon);
	if (rc) {
		dev_err(&spmi->dev,
			"Unable to intialize PON configurations\n");
		return rc;
	}

	sec_powerkey = device_create(sec_class, NULL, 0, NULL, "sec_powerkey");
	if (IS_ERR(sec_powerkey))
		pr_err("Failed to create device(sec_powerkey)!\n");
	ret = device_create_file(sec_powerkey, &dev_attr_sec_powerkey_pressed);
	if (ret) {
		pr_err("Failed to create device file in sysfs entries(%s)!\n",
			dev_attr_sec_powerkey_pressed.attr.name);
	}
	dev_set_drvdata(sec_powerkey, pon);

	return rc;
}

static int qpnp_pon_remove(struct spmi_device *spmi)
{
	struct qpnp_pon *pon = dev_get_drvdata(&spmi->dev);

	cancel_delayed_work_sync(&pon->bark_work);

	if (pon->pon_input)
		input_unregister_device(pon->pon_input);

	return 0;
}

static struct of_device_id spmi_match_table[] = {
	{ .compatible = "qcom,qpnp-power-on", },
	{}
};

static struct spmi_driver qpnp_pon_driver = {
	.driver		= {
		.name	= "qcom,qpnp-power-on",
		.of_match_table = spmi_match_table,
	},
	.probe		= qpnp_pon_probe,
	.remove		= __devexit_p(qpnp_pon_remove),
};

static int __init qpnp_pon_init(void)
{
	return spmi_driver_register(&qpnp_pon_driver);
}
#ifdef CONFIG_ARCH_MSM8226
subsys_initcall(qpnp_pon_init);
#else
module_init(qpnp_pon_init);
#endif

static void __exit qpnp_pon_exit(void)
{
	return spmi_driver_unregister(&qpnp_pon_driver);
}
module_exit(qpnp_pon_exit);

MODULE_DESCRIPTION("QPNP PMIC POWER-ON driver");
MODULE_LICENSE("GPL v2");
