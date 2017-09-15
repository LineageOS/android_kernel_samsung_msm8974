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
#include <linux/rtc.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/of_device.h>
#include <linux/spmi.h>
#include <linux/spinlock.h>
#include <linux/spmi.h>
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
#include <linux/qpnp/qpnp-adc.h>
#include <linux/power_supply.h>
#include <linux/battery/sec_charging_common.h>
#endif

#ifdef CONFIG_RTC_AUTO_PWRON
#include <linux/reboot.h>
extern int poweroff_charging;

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
#include <linux/sec_param.h>
#include <linux/param.h>

/* for alarm mode */
#define ALARM_MODE_NOMAL			(0x6A)
#define ALARM_MODE_BOOT_RTC		(0x7B)
#define SAPA_BOOTING_TIME		(60*3)
#endif
#endif

#ifdef CONFIG_SEC_PM
#include <linux/wakelock.h>
#endif

/* RTC/ALARM Register offsets */
#define REG_OFFSET_ALARM_RW	0x40
#define REG_OFFSET_ALARM_CTRL1	0x46
#define REG_OFFSET_ALARM_CTRL2	0x48
#define REG_OFFSET_RTC_WRITE	0x40
#define REG_OFFSET_RTC_CTRL	0x46
#define REG_OFFSET_RTC_READ	0x48
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
#define INT_ADD			0x10
#define BATT_PRES_IRQ		BIT(0)
#endif
#define REG_OFFSET_PERP_SUBTYPE	0x05

/* RTC_CTRL register bit fields */
#define BIT_RTC_ENABLE		BIT(7)
#define BIT_RTC_ALARM_ENABLE	BIT(7)
#define BIT_RTC_ABORT_ENABLE	BIT(0)
#define BIT_RTC_ALARM_CLEAR	BIT(0)

/* RTC/ALARM peripheral subtype values */
#define RTC_PERPH_SUBTYPE       0x1
#define ALARM_PERPH_SUBTYPE     0x3
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
#define RTC_BATT_PRES_IRQ	0x33
#endif

#define NUM_8_BIT_RTC_REGS	0x4

#define TO_SECS(arr)		(arr[0] | (arr[1] << 8) | (arr[2] << 16) | \
							(arr[3] << 24))

/* Module parameter to control power-on-alarm */
static bool poweron_alarm;
module_param(poweron_alarm, bool, 0644);
MODULE_PARM_DESC(poweron_alarm, "Enable/Disable power-on alarm");

#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
static enum power_supply_property pm8926_battery_props[] = {
        POWER_SUPPLY_PROP_PRESENT,
};
#endif

/* rtc driver internal structure */
struct qpnp_rtc {
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	bool battery_present;
#endif
	u8  rtc_ctrl_reg;
	u8  alarm_ctrl_reg1;
	u16 rtc_base;
	u16 alarm_base;
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	u16 bat_base;
#endif
	u32 rtc_write_enable;
	u32 rtc_alarm_powerup;
	int rtc_alarm_irq;
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	int bat_pres_irq;
	struct power_supply psy_pm8926;
#endif
	struct device *rtc_dev;
	struct rtc_device *rtc;
	struct spmi_device *spmi;
	spinlock_t alarm_ctrl_lock;

#if defined(CONFIG_RTC_AUTO_PWRON)
	bool lpm_mode;
	bool alarm_irq_flag;
	struct wake_lock alarm_wake_lock;
#endif	
};

#if defined(CONFIG_RTC_AUTO_PWRON)
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
static struct workqueue_struct*	sapa_workq;
static struct workqueue_struct*	sapa_check_workq;
static struct delayed_work		sapa_load_param_work;
static struct delayed_work		sapa_reboot_work;
static struct delayed_work		sapa_check_work;
static struct wake_lock			sapa_wakelock;
static int kparam_loaded, shutdown_loaded;
#endif
static struct rtc_wkalrm		sapa_saved_time;
static int						sapa_dev_suspend;

static void print_time(char* str, struct rtc_time *time, unsigned long sec)
{
	pr_info("%s: %4d-%02d-%02d %02d:%02d:%02d [%lu]\n", str,
		time->tm_year, time->tm_mon, time->tm_mday,
		time->tm_hour, time->tm_min, time->tm_sec, sec);
}
#endif

#ifdef CONFIG_SEC_PM
static struct wake_lock			resume_wakelock;
#endif

static int qpnp_read_wrapper(struct qpnp_rtc *rtc_dd, u8 *rtc_val,
			u16 base, int count)
{
	int rc;
	struct spmi_device *spmi = rtc_dd->spmi;

	rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, base, rtc_val,
					count);
	if (rc) {
		dev_err(rtc_dd->rtc_dev, "SPMI read failed\n");
		return rc;
	}
	return 0;
}

static int qpnp_write_wrapper(struct qpnp_rtc *rtc_dd, u8 *rtc_val,
			u16 base, int count)
{
	int rc;
	struct spmi_device *spmi = rtc_dd->spmi;

	rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, base, rtc_val,
					count);
	if (rc) {
		dev_err(rtc_dd->rtc_dev, "SPMI write failed\n");
		return rc;
	}

	return 0;
}

static int
qpnp_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	int rc;
	unsigned long secs, irq_flags;
	u8 value[4], reg = 0, alarm_enabled = 0, ctrl_reg;
	u8 rtc_disabled = 0, rtc_ctrl_reg;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);

	rtc_tm_to_time(tm, &secs);

	value[0] = secs & 0xFF;
	value[1] = (secs >> 8) & 0xFF;
	value[2] = (secs >> 16) & 0xFF;
	value[3] = (secs >> 24) & 0xFF;

	dev_info(dev, "Seconds value to be written to RTC = %lu\n", secs);

	spin_lock_irqsave(&rtc_dd->alarm_ctrl_lock, irq_flags);
	ctrl_reg = rtc_dd->alarm_ctrl_reg1;

	if (ctrl_reg & BIT_RTC_ALARM_ENABLE) {
		alarm_enabled = 1;
		ctrl_reg &= ~BIT_RTC_ALARM_ENABLE;
		rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
		if (rc) {
			dev_err(dev, "Write to ALARM ctrl reg failed\n");
			goto rtc_rw_fail;
		}
	} else
		spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);

	/*
	 * 32 bit seconds value is coverted to four 8 bit values
	 *	|<------  32 bit time value in seconds  ------>|
	 *      <- 8 bit ->|<- 8 bit ->|<- 8 bit ->|<- 8 bit ->|
	 *       ----------------------------------------------
	 *      | BYTE[3]  |  BYTE[2]  |  BYTE[1]  |  BYTE[0]  |
	 *       ----------------------------------------------
	 *
	 * RTC has four 8 bit registers for writting time in seconds:
	 *             WDATA[3], WDATA[2], WDATA[1], WDATA[0]
	 *
	 * Write to the RTC registers should be done in following order
	 * Clear WDATA[0] register
	 *
	 * Write BYTE[1], BYTE[2] and BYTE[3] of time to
	 * RTC WDATA[3], WDATA[2], WDATA[1] registers
	 *
	 * Write BYTE[0] of time to RTC WDATA[0] register
	 *
	 * Clearing BYTE[0] and writting in the end will prevent any
	 * unintentional overflow from WDATA[0] to higher bytes during the
	 * write operation
	 */

	/* Disable RTC H/w before writing on RTC register*/
	rtc_ctrl_reg = rtc_dd->rtc_ctrl_reg;
	if (rtc_ctrl_reg & BIT_RTC_ENABLE) {
		rtc_disabled = 1;
		rtc_ctrl_reg &= ~BIT_RTC_ENABLE;
		rc = qpnp_write_wrapper(rtc_dd, &rtc_ctrl_reg,
				rtc_dd->rtc_base + REG_OFFSET_RTC_CTRL, 1);
		if (rc) {
			dev_err(dev,
				"Disabling of RTC control reg failed"
					" with error:%d\n", rc);
			goto rtc_rw_fail;
		}
		rtc_dd->rtc_ctrl_reg = rtc_ctrl_reg;
	}

	/* Clear WDATA[0] */
	reg = 0x0;
	rc = qpnp_write_wrapper(rtc_dd, &reg,
				rtc_dd->rtc_base + REG_OFFSET_RTC_WRITE, 1);
	if (rc) {
		dev_err(dev, "Write to RTC reg failed\n");
		goto rtc_rw_fail;
	}

	/* Write to WDATA[3], WDATA[2] and WDATA[1] */
	rc = qpnp_write_wrapper(rtc_dd, &value[1],
			rtc_dd->rtc_base + REG_OFFSET_RTC_WRITE + 1, 3);
	if (rc) {
		dev_err(dev, "Write to RTC reg failed\n");
		goto rtc_rw_fail;
	}

	/* Write to WDATA[0] */
	rc = qpnp_write_wrapper(rtc_dd, value,
				rtc_dd->rtc_base + REG_OFFSET_RTC_WRITE, 1);
	if (rc) {
		dev_err(dev, "Write to RTC reg failed\n");
		goto rtc_rw_fail;
	}

	/* Enable RTC H/w after writing on RTC register*/
	if (rtc_disabled) {
		rtc_ctrl_reg |= BIT_RTC_ENABLE;
		rc = qpnp_write_wrapper(rtc_dd, &rtc_ctrl_reg,
				rtc_dd->rtc_base + REG_OFFSET_RTC_CTRL, 1);
		if (rc) {
			dev_err(dev,
				"Enabling of RTC control reg failed"
					" with error:%d\n", rc);
			goto rtc_rw_fail;
		}
		rtc_dd->rtc_ctrl_reg = rtc_ctrl_reg;
	}

	if (alarm_enabled) {
		ctrl_reg |= BIT_RTC_ALARM_ENABLE;
		rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
		if (rc) {
			dev_err(dev, "Write to ALARM ctrl reg failed\n");
			goto rtc_rw_fail;
		}
	}

	rtc_dd->alarm_ctrl_reg1 = ctrl_reg;

#ifdef CONFIG_RTC_AUTO_PWRON
	pr_info("%s : secs = %lu, h:m:s == %d:%d:%d, d/m/y = %d/%d/%d\n", __func__,
			secs, tm->tm_hour, tm->tm_min, tm->tm_sec,
			tm->tm_mday, tm->tm_mon, tm->tm_year);
#endif

rtc_rw_fail:
	if (alarm_enabled)
		spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);

	return rc;
}

static int
qpnp_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	int rc;
	u8 value[4], reg;
	unsigned long secs;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);

	rc = qpnp_read_wrapper(rtc_dd, value,
				rtc_dd->rtc_base + REG_OFFSET_RTC_READ,
				NUM_8_BIT_RTC_REGS);
	if (rc) {
		dev_err(dev, "Read from RTC reg failed\n");
		return rc;
	}

	/*
	 * Read the LSB again and check if there has been a carry over
	 * If there is, redo the read operation
	 */
	rc = qpnp_read_wrapper(rtc_dd, &reg,
				rtc_dd->rtc_base + REG_OFFSET_RTC_READ, 1);
	if (rc) {
		dev_err(dev, "Read from RTC reg failed\n");
		return rc;
	}

	if (reg < value[0]) {
		rc = qpnp_read_wrapper(rtc_dd, value,
				rtc_dd->rtc_base + REG_OFFSET_RTC_READ,
				NUM_8_BIT_RTC_REGS);
		if (rc) {
			dev_err(dev, "Read from RTC reg failed\n");
			return rc;
		}
	}

	secs = TO_SECS(value);

	rtc_time_to_tm(secs, tm);

	rc = rtc_valid_tm(tm);
	if (rc) {
		dev_err(dev, "Invalid time read from RTC\n");
		return rc;
	}

	dev_dbg(dev, "secs = %lu, h:m:s == %d:%d:%d, d/m/y = %d/%d/%d\n",
			secs, tm->tm_hour, tm->tm_min, tm->tm_sec,
			tm->tm_mday, tm->tm_mon, tm->tm_year);

	return 0;
}

static int
qpnp_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int rc;
	u8 value[4], ctrl_reg;
	unsigned long secs, secs_rtc, irq_flags;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);
	struct rtc_time rtc_tm;

	rtc_tm_to_time(&alarm->time, &secs);

	/*
	 * Read the current RTC time and verify if the alarm time is in the
	 * past. If yes, return invalid
	 */
	rc = qpnp_rtc_read_time(dev, &rtc_tm);
	if (rc) {
		dev_err(dev, "Unable to read RTC time\n");
		return -EINVAL;
	}

	rtc_tm_to_time(&rtc_tm, &secs_rtc);
	if (secs < secs_rtc) {
		dev_err(dev, "Trying to set alarm in the past\n");
		return -EINVAL;
	}

#ifdef CONFIG_RTC_AUTO_PWRON
	if ( sapa_saved_time.enabled ) {
		unsigned long secs_pwron;

		/* If there are power on alarm before alarm time, ignore alarm */
		rtc_tm_to_time(&sapa_saved_time.time, &secs_pwron);

		print_time("[SAPA] rtc ", &rtc_tm, secs_rtc);
		print_time("[SAPA] sapa", &sapa_saved_time.time, secs_pwron);
		print_time("[SAPA] alrm", &alarm->time, secs);

		if ( secs_pwron <= secs_rtc && secs_rtc <= secs_pwron+SAPA_BOOTING_TIME ) {
			if ( poweroff_charging ) {
				wake_lock(&sapa_wakelock);
				rtc_dd->alarm_irq_flag = true;
				pr_info("%s [SAPA] Restart(alarm)\n",__func__);
				queue_delayed_work(sapa_workq, &sapa_reboot_work, (1*HZ));
				return -EINVAL;
			}
		}
		if ( secs_rtc < secs_pwron && secs_pwron < secs ) {
			pr_info("[SAPA] override with SAPA\n");
			memcpy(alarm, &sapa_saved_time, sizeof(struct rtc_wkalrm));
			secs = secs_pwron;
		}
	}
#endif

	value[0] = secs & 0xFF;
	value[1] = (secs >> 8) & 0xFF;
	value[2] = (secs >> 16) & 0xFF;
	value[3] = (secs >> 24) & 0xFF;

	spin_lock_irqsave(&rtc_dd->alarm_ctrl_lock, irq_flags);

	rc = qpnp_write_wrapper(rtc_dd, value,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
				NUM_8_BIT_RTC_REGS);
	if (rc) {
		dev_err(dev, "Write to ALARM reg failed\n");
		goto rtc_rw_fail;
	}

	ctrl_reg = (alarm->enabled) ?
			(rtc_dd->alarm_ctrl_reg1 | BIT_RTC_ALARM_ENABLE) :
			(rtc_dd->alarm_ctrl_reg1 & ~BIT_RTC_ALARM_ENABLE);

	rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
	if (rc) {
		dev_err(dev, "Write to ALARM cntrol reg failed\n");
		goto rtc_rw_fail;
	}

	rtc_dd->alarm_ctrl_reg1 = ctrl_reg;

	dev_dbg(dev, "Alarm Set for h:r:s=%d:%d:%d, d/m/y=%d/%d/%d\n",
			alarm->time.tm_hour, alarm->time.tm_min,
			alarm->time.tm_sec, alarm->time.tm_mday,
			alarm->time.tm_mon, alarm->time.tm_year);
rtc_rw_fail:
	spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);
	return rc;
}

static int
qpnp_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int rc;
	u8 value[4];
	unsigned long secs;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);

	rc = qpnp_read_wrapper(rtc_dd, value,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
				NUM_8_BIT_RTC_REGS);
	if (rc) {
		dev_err(dev, "Read from ALARM reg failed\n");
		return rc;
	}

	secs = TO_SECS(value);
	rtc_time_to_tm(secs, &alarm->time);

	rc = rtc_valid_tm(&alarm->time);
	if (rc) {
		dev_err(dev, "Invalid time read from RTC\n");
		return rc;
	}

	dev_dbg(dev, "Alarm set for - h:r:s=%d:%d:%d, d/m/y=%d/%d/%d\n",
		alarm->time.tm_hour, alarm->time.tm_min,
				alarm->time.tm_sec, alarm->time.tm_mday,
				alarm->time.tm_mon, alarm->time.tm_year);

	return 0;
}


static int
qpnp_rtc_alarm_irq_enable(struct device *dev, unsigned int enabled)
{
	int rc;
	unsigned long irq_flags;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);
	u8 ctrl_reg;
	u8 value[4] = {0};

#ifdef CONFIG_RTC_AUTO_PWRON
	pr_info("[SAPA] irq=%d\n", enabled);
#endif

	spin_lock_irqsave(&rtc_dd->alarm_ctrl_lock, irq_flags);
	ctrl_reg = rtc_dd->alarm_ctrl_reg1;
	ctrl_reg = enabled ? (ctrl_reg | BIT_RTC_ALARM_ENABLE) :
				(ctrl_reg & ~BIT_RTC_ALARM_ENABLE);

	rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
	if (rc) {
		dev_err(dev, "Write to ALARM control reg failed\n");
		goto rtc_rw_fail;
	}

	rtc_dd->alarm_ctrl_reg1 = ctrl_reg;

	/* Clear Alarm register */
	if (!enabled) {
		rc = qpnp_write_wrapper(rtc_dd, value,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
			NUM_8_BIT_RTC_REGS);
		if (rc)
			dev_err(dev, "Clear ALARM value reg failed\n");
	}

rtc_rw_fail:
	spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);
	return rc;
}

#ifdef CONFIG_RTC_AUTO_PWRON
static void sapa_reboot(struct work_struct *work)
{
	//machine_restart(NULL);
	kernel_restart(NULL);
	//panic("Test panic");
}

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
static struct device *			sapa_rtc_dev;
static int qpnp_rtc0_resetbootalarm(struct device *dev);
static void sapa_load_kparam(struct work_struct *work)
{
	int temp1, temp2, temp3;
	unsigned long pwron_time=(unsigned long)0;
	bool rc, kparam_ok = true;
	static unsigned int kparam_count = (unsigned int)0;

	rc = sec_get_param(param_index_boot_alarm_set, &temp1);
	if(!rc)
		kparam_ok = false;
	rc = sec_get_param(param_index_boot_alarm_value_l, &temp2);
	if(!rc)
		kparam_ok = false;
	rc = sec_get_param(param_index_boot_alarm_value_h, &temp3);
	if(!rc)
		kparam_ok = false;

	if(!kparam_ok) {
		if(kparam_count < 3) {
			queue_delayed_work(sapa_workq, &sapa_load_param_work, (5*HZ));
			kparam_count++;
			pr_err("[SAPA] %s fail, count=%d\n", __func__, kparam_count);
			return ;
		} else {
			pr_err("[SAPA] %s final fail, just go on\n", __func__);
		}
	}

	pwron_time = temp3<<4 | temp2;

	pr_info("[SAPA] %s %x %lu\n", __func__, temp1, pwron_time);
	if ( temp1 == ALARM_MODE_BOOT_RTC )
		sapa_saved_time.enabled = 1;
	else
		sapa_saved_time.enabled = 0;

	kparam_loaded = 1;

	rtc_time_to_tm( pwron_time, &sapa_saved_time.time );
	print_time("[SAPA] saved_time", &sapa_saved_time.time, pwron_time);
	/* Bug fix : USB cable or IRQ is disabled in LPM chg */
	qpnp_rtc0_resetbootalarm(sapa_rtc_dev);
}
#endif

static void sapa_store_kparam(struct rtc_wkalrm *alarm)
{
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	//int temp1, temp2, temp3;
	int MSB=0, LSB=0;
	int alarm_mode = 0;
	unsigned long secs;

	if ( alarm == &sapa_saved_time ) {
		pr_err("[SAPA] %s: already was written\n", __func__);
		return ;
	}

	if ( alarm->enabled ) {
		rtc_tm_to_time(&alarm->time, &secs);
		LSB = (int)secs;
		MSB = (int)(secs>>4);

		alarm_mode = ALARM_MODE_BOOT_RTC;
		sec_set_param(param_index_boot_alarm_set, &alarm_mode);
		sec_set_param(param_index_boot_alarm_value_l, &LSB);
		sec_set_param(param_index_boot_alarm_value_h, &MSB);
		pr_info("[SAPA] %s %x/%x/%x\n", __func__, alarm_mode, LSB, MSB);

		#if 0 // for debugging
		sec_get_param(param_index_boot_alarm_set,&temp1);
		sec_get_param(param_index_boot_alarm_value_l, &temp2);
		sec_get_param(param_index_boot_alarm_value_h, &temp3);
		pr_info( "sec_set_param [%x] [%x] [%x] -- feedback\n", temp1, temp2, temp3);
		#endif
	}
	else {
		alarm_mode = ALARM_MODE_NOMAL;
		sec_set_param(param_index_boot_alarm_set, &alarm_mode);
		pr_info("[SAPA] %s clear\n", __func__);
	}
#endif
}

#ifdef CONFIG_RTC_AUTO_PWRON
static void
sapa_check_alarm(struct work_struct *work)
{
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(sapa_rtc_dev);

	pr_info("%s [SAPA] : lpm_mode:(%d)\n", __func__, rtc_dd->lpm_mode);

	if ( poweroff_charging && sapa_saved_time.enabled) {
		struct rtc_time now;
		struct rtc_wkalrm alarm;
		unsigned long curr_time, alarm_time, pwron_time;

		/* To wake up rtc device */
		wake_lock_timeout(&sapa_wakelock, HZ/2 );

		qpnp_rtc_read_time(rtc_dd->rtc_dev, &now);
		rtc_tm_to_time(&now, &curr_time);

		qpnp_rtc_read_alarm(rtc_dd->rtc_dev, &alarm);
		rtc_tm_to_time(&alarm.time, &alarm_time);

		rtc_tm_to_time(&sapa_saved_time.time, &pwron_time);

		pr_info("%s [SAPA] curr_time: %lu\n",__func__, curr_time);
		pr_info("%s [SAPA] pmic_time: %lu\n",__func__, alarm_time);
		pr_info("%s [SAPA] pwrontime: %lu [%d]\n",__func__, pwron_time, sapa_saved_time.enabled);

		if ( pwron_time <= curr_time && curr_time <= pwron_time+SAPA_BOOTING_TIME )  {
			wake_lock(&sapa_wakelock);
			rtc_dd->alarm_irq_flag = true;
			pr_info("%s [SAPA] Restart since RTC \n",__func__);
			queue_delayed_work(sapa_workq, &sapa_reboot_work, (1*HZ));
		}
		else {
			pr_info("%s [SAPA] not power on alarm.\n", __func__);
			if (!sapa_dev_suspend) {
				qpnp_rtc0_resetbootalarm(rtc_dd->rtc_dev);
				queue_delayed_work(sapa_check_workq, &sapa_check_work, (60*HZ));
			}
		}
	}
}
#endif

static int
sapa_rtc_getalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	struct rtc_time b;
	int ret = 0;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);
	unsigned long secs_alrm, secs_rtc;

	/* read rtc time */
	if ( qpnp_rtc_read_time(dev, &b) ) {
		pr_err("%s [SAPA] : read time failed.\n", __func__);
		ret = -EINVAL;
	}
	memcpy(alarm, &sapa_saved_time, sizeof(struct rtc_wkalrm));

	if (rtc_dd->alarm_irq_flag)
		alarm->enabled = 0x1;
	else
		alarm->enabled = 0x0;
	
	pr_info("%s [SAPA] : %d, %d\n",__func__,rtc_dd->lpm_mode, alarm->enabled);
	
	if(poweroff_charging && sapa_saved_time.enabled)
	{
		rtc_tm_to_time(&b, &secs_rtc);
		rtc_tm_to_time(&alarm->time, &secs_alrm);

		if ( secs_alrm <= secs_rtc && secs_rtc <= secs_alrm+SAPA_BOOTING_TIME )
		{
			rtc_dd->alarm_irq_flag = true;
			pr_info("%s [SAPA] : it will be reboot \n",__func__);
		}
	}

	if ( !ret ) {
		pr_info("[SAPA] %s: [ALRM] %d-%d-%d %d:%d:%d \n", __func__,
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
			alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);
		pr_info("[SAPA] %s: [RTC ] %d-%d-%d %d:%d:%d \n", __func__,
			b.tm_year, b.tm_mon, b.tm_mday,
			b.tm_hour, b.tm_min, b.tm_sec);
	}

	return rtc_dd->lpm_mode;
}

static int
sapa_rtc_setalarm(struct device *dev, struct rtc_wkalrm *alarm)
{
	int rc;
	u8 value[4] = {0,}, ctrl_reg;
	unsigned long secs, secs_rtc;//, irq_flags;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);
	struct rtc_time rtc_tm;
	
	if (!alarm->enabled) {
		pr_info("[SAPA] Try to clear :  %4d-%02d-%02d %02d:%02d:%02d\n",
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
			alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);

		if(poweroff_charging && !kparam_loaded && shutdown_loaded){
			pr_info("%s [SAPA] without loading kparam, it will be shutdown. No need to reset the alarm!! \n",__func__);

			ctrl_reg = (rtc_dd->alarm_ctrl_reg1 | BIT_RTC_ALARM_ENABLE);
			rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
			
			if (rc) {
				dev_err(dev, "Write to ALARM cntrol reg failed\n");
				goto rtc_rw_fail;
			}
			
			return 0;
		}

		rc = qpnp_write_wrapper(rtc_dd, value,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
								NUM_8_BIT_RTC_REGS);
		if (rc < 0) {
			pr_err("[SAPA] Write to RTC ALARM registers failed\n");
			goto rtc_rw_fail;
		}
		
		sapa_saved_time.enabled = 0;  // disable pwr on alarm to prevent retrying
		sapa_store_kparam(alarm);

		ctrl_reg = (alarm->enabled) ?
			(rtc_dd->alarm_ctrl_reg1 | BIT_RTC_ALARM_ENABLE) :
			(rtc_dd->alarm_ctrl_reg1 & ~BIT_RTC_ALARM_ENABLE);

		rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);

		if (rc) {
		dev_err(dev, "Write to ALARM cntrol reg failed\n");
			goto rtc_rw_fail;
		}

		rtc_dd->alarm_ctrl_reg1 = ctrl_reg;

		/* read boot alarm */
		rc = qpnp_rtc_read_alarm(dev, alarm);
		if ( rc < 0 ) {
			pr_err("[SAPA] read failed.\n");
			return rc;
		}
		pr_info("[SAPA] -> %4d-%02d-%02d %02d:%02d:%02d\n",
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
			alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);
	}
	else
	{
		pr_info("[SAPA] <- %4d-%02d-%02d %02d:%02d:%02d\n",
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
			alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);

		rtc_tm_to_time(&alarm->time, &secs);

		/*
		 * Read the current RTC time and verify if the alarm time is in the
		 * past. If yes, return invalid.
		 */
		rc = qpnp_rtc_read_time(dev, &rtc_tm);
		if (rc < 0) {
			pr_err("[SAPA] Unable to read RTC time\n");
			return -EINVAL;
		}

		rtc_tm_to_time(&rtc_tm, &secs_rtc);
		if ( secs <= secs_rtc && secs_rtc <= secs+SAPA_BOOTING_TIME ) {
			if ( poweroff_charging ) {
				wake_lock(&sapa_wakelock);
				rtc_dd->alarm_irq_flag = true;
				pr_info("%s [SAPA] Restart(alarm)\n",__func__);
				queue_delayed_work(sapa_workq, &sapa_reboot_work, (10*HZ));
			}
			else if (shutdown_loaded) {
				pr_info("[SAPA] adjust to rtc+20s\n");
				secs = secs_rtc + 10;
			}
		}
		else if ( secs+SAPA_BOOTING_TIME < secs_rtc ) {
			pr_err("[SAPA] Trying to set alarm in the past\n");
			sapa_saved_time.enabled = 0;  // disable pwr on alarm to prevent retrying
			sapa_store_kparam(alarm);
			return -EINVAL;
		}

		value[0] = secs & 0xFF;
		value[1] = (secs >> 8) & 0xFF;
		value[2] = (secs >> 16) & 0xFF;
		value[3] = (secs >> 24) & 0xFF;

	    //spin_lock_irqsave(&rtc_dd->alarm_ctrl_lock, irq_flags);

	    rc = qpnp_write_wrapper(rtc_dd, value,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
								NUM_8_BIT_RTC_REGS);
		if (rc < 0) {
			pr_err("[SAPA] Write to RTC ALARM registers failed\n");
			goto rtc_rw_fail;
		}

	    ctrl_reg = (alarm->enabled) ?
			(rtc_dd->alarm_ctrl_reg1 | BIT_RTC_ALARM_ENABLE) :
			(rtc_dd->alarm_ctrl_reg1 & ~BIT_RTC_ALARM_ENABLE);

		rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
		if (rc) {
			dev_err(dev, "Write to ALARM cntrol reg failed\n");
				goto rtc_rw_fail;
			}

		rtc_dd->alarm_ctrl_reg1 = ctrl_reg;

		if ( alarm != &sapa_saved_time ) {
			memcpy(&sapa_saved_time, alarm, sizeof(struct rtc_wkalrm));
			sapa_store_kparam(alarm);
			pr_info("[SAPA] updated\n");
		}
	}

	/* read boot alarm */
	rc = qpnp_rtc_read_alarm(dev, alarm);
	if ( rc < 0 ) {
		pr_err("[SAPA] read failed.\n");
			return rc;
	}
	pr_info("[SAPA] -> %4d-%02d-%02d %02d:%02d:%02d\n",
			alarm->time.tm_year, alarm->time.tm_mon, alarm->time.tm_mday,
			alarm->time.tm_hour, alarm->time.tm_min, alarm->time.tm_sec);
	if ( alarm != &sapa_saved_time )
		qpnp_rtc_read_time(dev,&(alarm->time));

rtc_rw_fail:
	//spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);
	return rc;
}

static int qpnp_rtc0_resetbootalarm(struct device *dev)
{
	pr_info("[SAPA] rewrite [%d]\n", sapa_saved_time.enabled);
	return sapa_rtc_setalarm(dev, &sapa_saved_time);
}
#endif /*CONFIG_RTC_AUTO_PWRON*/

static struct rtc_class_ops qpnp_rtc_ops = {
	.read_time = qpnp_rtc_read_time,
	.set_alarm = qpnp_rtc_set_alarm,
	.read_alarm = qpnp_rtc_read_alarm,
#ifdef CONFIG_RTC_AUTO_PWRON
	.read_bootalarm = sapa_rtc_getalarm,
	.set_bootalarm  = sapa_rtc_setalarm,
#endif /*CONFIG_RTC_AUTO_PWRON*/
	.alarm_irq_enable = qpnp_rtc_alarm_irq_enable,
};

static irqreturn_t qpnp_alarm_trigger(int irq, void *dev_id)
{
	struct qpnp_rtc *rtc_dd = dev_id;
	u8 ctrl_reg;
	int rc;
	unsigned long irq_flags;

	rtc_update_irq(rtc_dd->rtc, 1, RTC_IRQF | RTC_AF);

	spin_lock_irqsave(&rtc_dd->alarm_ctrl_lock, irq_flags);

	/* Clear the alarm enable bit */
	ctrl_reg = rtc_dd->alarm_ctrl_reg1;
	ctrl_reg &= ~BIT_RTC_ALARM_ENABLE;

	rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
	if (rc) {
		spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);
		dev_err(rtc_dd->rtc_dev,
				"Write to ALARM control reg failed\n");
		goto rtc_alarm_handled;
	}

	rtc_dd->alarm_ctrl_reg1 = ctrl_reg;
	spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);

	/* Set ALARM_CLR bit */
	ctrl_reg = 0x1;
	rc = qpnp_write_wrapper(rtc_dd, &ctrl_reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL2, 1);
	if (rc)
		dev_err(rtc_dd->rtc_dev,
				"Write to ALARM control reg failed\n");

#ifdef CONFIG_RTC_AUTO_PWRON
	if ( poweroff_charging )
		pr_info("%s [SAPA] : irq(%d), lpm_mode\n", __func__, irq);

	if ( poweroff_charging && sapa_saved_time.enabled) {
		struct rtc_time now;
		struct rtc_wkalrm alarm;
		unsigned long curr_time, alarm_time, pwron_time;

		/* To wake up rtc device */
		wake_lock_timeout(&sapa_wakelock, HZ/2 );

		qpnp_rtc_read_time(rtc_dd->rtc_dev, &now);
		rtc_tm_to_time(&now, &curr_time);

		qpnp_rtc_read_alarm(rtc_dd->rtc_dev, &alarm);
		rtc_tm_to_time(&alarm.time, &alarm_time);

		rtc_tm_to_time(&sapa_saved_time.time, &pwron_time);

		pr_info("%s [SAPA] curr_time: %lu\n",__func__, curr_time);
		pr_info("%s [SAPA] pmic_time: %lu\n",__func__, alarm_time);
		pr_info("%s [SAPA] pwrontime: %lu [%d]\n",__func__, pwron_time, sapa_saved_time.enabled);

		if ( pwron_time <= curr_time && curr_time <= pwron_time+SAPA_BOOTING_TIME )  {
			wake_lock(&sapa_wakelock);
			rtc_dd->alarm_irq_flag = true;
			pr_info("%s [SAPA] Restart since RTC \n",__func__);
			queue_delayed_work(sapa_workq, &sapa_reboot_work, (1*HZ));
		}
		else {
			pr_info("%s [SAPA] not power on alarm.\n", __func__);
			if (!sapa_dev_suspend)
				qpnp_rtc0_resetbootalarm(rtc_dd->rtc_dev);
		}
	}
#endif

rtc_alarm_handled:
	return IRQ_HANDLED;
}

#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
static int pm8926_bat_set_property(struct power_supply *psy,
                                enum power_supply_property psp,
                                const union power_supply_propval *val)
{
        struct qpnp_rtc *rtc_dd =
                container_of(psy, struct qpnp_rtc, psy_pm8926);

       switch (psp) {
       case POWER_SUPPLY_PROP_PRESENT:
		if(val->intval != rtc_dd->battery_present)
                pr_debug("Cannot change value battery_present (%d) \n",rtc_dd->battery_present);

	default:
                return -EINVAL;
        }

        return 0;

}

static int pm8926_bat_get_property(struct power_supply *psy,
                                enum power_supply_property psp,
                                union power_supply_propval *val)
{
        struct qpnp_rtc *rtc_dd =
                container_of(psy, struct qpnp_rtc, psy_pm8926);

        switch (psp) {
        case POWER_SUPPLY_PROP_PRESENT:
                val->intval = rtc_dd->battery_present;
		pr_debug("rtc: battery_present = %d \n",rtc_dd->battery_present);
                break;

        default:
                return -EINVAL;
        }

        return 0;

}


static irqreturn_t qpnp_batt_pres_irq_handler(int irq, void *dev_id)
{
	struct qpnp_rtc *rtc_dd = dev_id;
	union power_supply_propval val;
	u8 reg;
	int rc;

	pr_info("##############################\n");
	pr_info("%s [SAPA] BAT_PRES TRIGGER\n",__func__);
	pr_info("##############################\n");

	rc = qpnp_read_wrapper(rtc_dd, &reg,
				rtc_dd->bat_base + INT_ADD, 1);
	if (rc < 0) {
		pr_err("%s qpnp read failed!\n",__func__);
	}

	reg = reg & BATT_PRES_IRQ;

	if(reg){
		pr_info("%s [SAPA] BATTERY PRESENT\n",__func__);
		rtc_dd->battery_present = 1;
	}else{
		pr_info("%s [SAPA] BATTERY ABSENT\n",__func__);
		rtc_dd->battery_present = 0;
	}

	val.intval = rtc_dd->battery_present;

	psy_do_property("battery", set,
                                POWER_SUPPLY_PROP_PRESENT, val);

	return IRQ_HANDLED;
}
#endif

static int __devinit qpnp_rtc_probe(struct spmi_device *spmi)
{
	int rc;
	u8 subtype;
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	u8 reg;
#endif
	struct qpnp_rtc *rtc_dd;
	struct resource *resource;
	struct spmi_resource *spmi_resource;
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	u8 alarm_reg=0;
	u8 value[4];
	unsigned long rtc_secs, pmic_secs;
#endif

	rtc_dd = devm_kzalloc(&spmi->dev, sizeof(*rtc_dd), GFP_KERNEL);
	if (rtc_dd == NULL) {
		dev_err(&spmi->dev, "Unable to allocate memory!\n");
		return -ENOMEM;
	}

	/* Get the rtc write property */
	rc = of_property_read_u32(spmi->dev.of_node, "qcom,qpnp-rtc-write",
						&rtc_dd->rtc_write_enable);
	if (rc && rc != -EINVAL) {
		dev_err(&spmi->dev,
			"Error reading rtc_write_enable property %d\n", rc);
		return rc;
	}

	rc = of_property_read_u32(spmi->dev.of_node,
						"qcom,qpnp-rtc-alarm-pwrup",
						&rtc_dd->rtc_alarm_powerup);
	if (rc && rc != -EINVAL) {
		dev_err(&spmi->dev,
			"Error reading rtc_alarm_powerup property %d\n", rc);
		return rc;
	}

	/* Initialise spinlock to protect RTC control register */
	spin_lock_init(&rtc_dd->alarm_ctrl_lock);

	rtc_dd->rtc_dev = &(spmi->dev);
	rtc_dd->spmi = spmi;

	/* Get RTC/ALARM resources */
	spmi_for_each_container_dev(spmi_resource, spmi) {
		if (!spmi_resource) {
			dev_err(&spmi->dev,
				"%s: rtc_alarm: spmi resource absent!\n",
				__func__);
			rc = -ENXIO;
			goto fail_rtc_enable;
		}

		resource = spmi_get_resource(spmi, spmi_resource,
							IORESOURCE_MEM, 0);
		if (!(resource && resource->start)) {
			dev_err(&spmi->dev,
				"%s: node %s IO resource absent!\n",
				__func__, spmi->dev.of_node->full_name);
			rc = -ENXIO;
			goto fail_rtc_enable;
		}

		rc = qpnp_read_wrapper(rtc_dd, &subtype,
				resource->start + REG_OFFSET_PERP_SUBTYPE, 1);
		if (rc) {
			dev_err(&spmi->dev,
				"Peripheral subtype read failed\n");
			goto fail_rtc_enable;
		}

		switch (subtype) {
		case RTC_PERPH_SUBTYPE:
			rtc_dd->rtc_base = resource->start;
			break;
		case ALARM_PERPH_SUBTYPE:
			rtc_dd->alarm_base = resource->start;
			rtc_dd->rtc_alarm_irq =
				spmi_get_irq(spmi, spmi_resource, 0);
			if (rtc_dd->rtc_alarm_irq < 0) {
				dev_err(&spmi->dev, "ALARM IRQ absent\n");
				rc = -ENXIO;
				goto fail_rtc_enable;
			}
			break;
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
		case RTC_BATT_PRES_IRQ:
			rtc_dd->bat_base = resource->start;
			rtc_dd->bat_pres_irq = spmi_get_irq(spmi, spmi_resource, 0);
			break;
#endif
		default:
			dev_err(&spmi->dev, "Invalid peripheral subtype\n");
			rc = -EINVAL;
			goto fail_rtc_enable;

		}
	}

	rc = qpnp_read_wrapper(rtc_dd, &rtc_dd->rtc_ctrl_reg,
				rtc_dd->rtc_base + REG_OFFSET_RTC_CTRL, 1);
	if (rc) {
		dev_err(&spmi->dev,
			"Read from RTC control reg failed\n");
		goto fail_rtc_enable;
	}

	if (!(rtc_dd->rtc_ctrl_reg & BIT_RTC_ENABLE)) {
		dev_err(&spmi->dev,
			"RTC h/w disabled, rtc not registered\n");
		goto fail_rtc_enable;
	}

	rc = qpnp_read_wrapper(rtc_dd, &rtc_dd->alarm_ctrl_reg1,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
	if (rc) {
		dev_err(&spmi->dev,
			"Read from  Alarm control reg failed\n");
		goto fail_rtc_enable;
	}
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	alarm_reg = rtc_dd->alarm_ctrl_reg1;
#endif
	/* Enable abort enable feature */
	rtc_dd->alarm_ctrl_reg1 |= BIT_RTC_ABORT_ENABLE;
	rc = qpnp_write_wrapper(rtc_dd, &rtc_dd->alarm_ctrl_reg1,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
	if (rc) {
		dev_err(&spmi->dev, "SPMI write failed!\n");
		goto fail_rtc_enable;
	}
#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	rc = qpnp_read_wrapper(rtc_dd, value,
			rtc_dd->rtc_base + REG_OFFSET_RTC_READ,	NUM_8_BIT_RTC_REGS);
	if (rc) pr_err("Read from RTC reg failed\n");
	rtc_secs = TO_SECS(value);

	rc = qpnp_read_wrapper(rtc_dd, value,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_RW, NUM_8_BIT_RTC_REGS);
	if (rc) pr_err("Read from ALARM reg failed\n");
	pmic_secs = TO_SECS(value);

	pr_info("[SAPA] alarm_reg=%02x, rtc=%lu pmic=%lu\n", alarm_reg, rtc_secs, pmic_secs);
#endif

#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	reg = 0xFF;
	spmi_ext_register_writel(rtc_dd->spmi->ctrl, rtc_dd->spmi->sid, rtc_dd->bat_base + 0x48, &reg, 1);
#endif

	if (rtc_dd->rtc_write_enable == true)
		qpnp_rtc_ops.set_time = qpnp_rtc_set_time;

	dev_set_drvdata(&spmi->dev, rtc_dd);

	/* Register the RTC device */
	rtc_dd->rtc = rtc_device_register("qpnp_rtc", &spmi->dev,
						&qpnp_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc_dd->rtc)) {
		dev_err(&spmi->dev, "%s: RTC registration failed (%ld)\n",
					__func__, PTR_ERR(rtc_dd->rtc));
		rc = PTR_ERR(rtc_dd->rtc);
		goto fail_rtc_enable;
	}

	/* Request the alarm IRQ */
	rc = request_any_context_irq(rtc_dd->rtc_alarm_irq,
				 qpnp_alarm_trigger, IRQF_TRIGGER_RISING,
				 "qpnp_rtc_alarm", rtc_dd);
	if (rc) {
		dev_err(&spmi->dev, "Request IRQ failed (%d)\n", rc);
		goto fail_req_irq;
        }

#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	rc = request_any_context_irq(rtc_dd->bat_pres_irq,
				 qpnp_batt_pres_irq_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING
				| IRQF_SHARED | IRQF_ONESHOT,
				"batt-pres", rtc_dd);
	if (rc) {
		dev_err(&spmi->dev, "Request IRQ BATT_PRES failed (%d)\n", rc);
		goto fail_req_irq;
	}
#endif

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	sapa_rtc_dev = rtc_dd->rtc_dev;
	sapa_workq = create_singlethread_workqueue("pwron_alarm_resume");
	if (sapa_workq == NULL) {
		pr_err("[SAPA] pwron_alarm work creating failed (%d)\n", rc);
	}
	wake_lock_init(&sapa_wakelock, WAKE_LOCK_SUSPEND, "alarm_trigger");
#endif

#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	/* Initialize battery present */
	rtc_dd->battery_present = 1;
	qpnp_batt_pres_irq_handler(rtc_dd->bat_pres_irq, rtc_dd);

	rtc_dd->psy_pm8926.name = "pm8926",
        rtc_dd->psy_pm8926.type = POWER_SUPPLY_TYPE_BATTERY,
        rtc_dd->psy_pm8926.properties = pm8926_battery_props,
        rtc_dd->psy_pm8926.num_properties = ARRAY_SIZE(pm8926_battery_props),
        rtc_dd->psy_pm8926.get_property = pm8926_bat_get_property,
        rtc_dd->psy_pm8926.set_property = pm8926_bat_set_property,

	rc = power_supply_register(&spmi->dev, &rtc_dd->psy_pm8926);
        if (rc) {
		dev_err(&spmi->dev,
                        "%s: Failed to Register psy_pm8926 \n", __func__);
                goto fail_supply_unreg_pm8926;
        }

	dev_dbg(&spmi->dev,"Power-supply pm8926 registered successfully \n");
#endif

#ifdef CONFIG_SEC_PM
	wake_lock_init(&resume_wakelock, WAKE_LOCK_SUSPEND, "resume_wakelock");
#endif
	device_init_wakeup(&spmi->dev, 1);
	enable_irq_wake(rtc_dd->rtc_alarm_irq);
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	enable_irq_wake(rtc_dd->bat_pres_irq);
#endif

	dev_err(&spmi->dev, "Probe success !!\n");

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	rtc_dd->lpm_mode = poweroff_charging;
	rtc_dd->alarm_irq_flag = false;
	/* To read saved power on alarm time */
	if ( poweroff_charging ) {
		sapa_check_workq = create_singlethread_workqueue("pwron_alarm_check");
		if (sapa_check_workq == NULL) {
			pr_err("[SAPA] pwron_alarm_check work creating failed (%d)\n", rc);
		}
		INIT_DELAYED_WORK(&sapa_load_param_work, sapa_load_kparam);
		INIT_DELAYED_WORK(&sapa_reboot_work, sapa_reboot);
		INIT_DELAYED_WORK(&sapa_check_work, sapa_check_alarm);
		queue_delayed_work(sapa_workq, &sapa_load_param_work, (5*HZ));
		queue_delayed_work(sapa_check_workq, &sapa_check_work, (60*HZ));
	}
#endif
	return 0;

fail_req_irq:
	rtc_device_unregister(rtc_dd->rtc);
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
fail_supply_unreg_pm8926:
	power_supply_unregister(&rtc_dd->psy_pm8926);
#endif
fail_rtc_enable:
	dev_set_drvdata(&spmi->dev, NULL);

	return rc;
}

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
static int qpnp_rtc_auto_pwron_resume(struct device *dev)
{
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		disable_irq_wake(rtc_dd->rtc_alarm_irq);

	sapa_dev_suspend = 0;
	qpnp_rtc0_resetbootalarm(dev);
	if(rtc_dd->lpm_mode==1)
		queue_delayed_work(sapa_check_workq, &sapa_check_work, (1*HZ));

	pr_info("%s\n",__func__);
	return 0;
}

static int qpnp_rtc_auto_pwron_suspend(struct device *dev)
{
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(dev);

	if (device_may_wakeup(dev))
		enable_irq_wake(rtc_dd->rtc_alarm_irq);

	sapa_dev_suspend = 1;
	if(rtc_dd->lpm_mode==1)
		cancel_delayed_work_sync(&sapa_check_work);

	pr_info("%s\n",__func__);
	return 0;
}
#endif

#ifdef CONFIG_SEC_PM
static int qpnp_rtc_resume(struct device *dev)
{
	pr_info("%s\n",__func__);
	wake_lock_timeout(&resume_wakelock, HZ/10);

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	qpnp_rtc_auto_pwron_resume(dev);
#endif

	return 0;
}

static int qpnp_rtc_suspend(struct device *dev)
{
	pr_info("%s\n",__func__);

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	qpnp_rtc_auto_pwron_suspend(dev);
#endif

	return 0;
}

static const struct dev_pm_ops qpnp_rtc_pm_ops = {
	.suspend = qpnp_rtc_suspend,
	.resume = qpnp_rtc_resume,
};
#endif

static int __devexit qpnp_rtc_remove(struct spmi_device *spmi)
{
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(&spmi->dev);

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	destroy_workqueue(sapa_workq);
#endif

	device_init_wakeup(&spmi->dev, 0);
	free_irq(rtc_dd->rtc_alarm_irq, rtc_dd);
#if defined(CONFIG_PM8926_BATTERY_CHECK_INTERRUPT)
	free_irq(rtc_dd->bat_pres_irq, rtc_dd);
#endif
	rtc_device_unregister(rtc_dd->rtc);
	dev_set_drvdata(&spmi->dev, NULL);

	return 0;
}

#ifdef CONFIG_RTC_AUTO_PWRON
static void qpnp_rtc_shutdown(struct spmi_device *spmi)
{
	u8 value[4] = {0};
	unsigned long secs;
	u8 ctrl_reg;
	int rc;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(&spmi->dev);
	
	shutdown_loaded = 1;
	qpnp_rtc0_resetbootalarm(&spmi->dev);

	/* Check if the RTC is on, else turn it on */
	rc = qpnp_read_wrapper(rtc_dd, &ctrl_reg,
				rtc_dd->rtc_base + REG_OFFSET_RTC_CTRL, 1);
	if (rc < 0) {
		dev_err(&spmi->dev, "%s qpnp read failed!\n",__func__);
	}

	rc = qpnp_read_wrapper(rtc_dd, value,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
				NUM_8_BIT_RTC_REGS);

	secs = value[0] | (value[1] << 8) | (value[2] << 16) \
						| (value[3] << 24);
	pr_info("%s : secs = %lu\n", __func__,secs);
	pr_info("%s RTC Register : %d \n", __func__, ctrl_reg);

#ifdef CONFIG_RTC_AUTO_PWRON_PARAM
	wake_lock_destroy(&sapa_wakelock);
#endif
#ifdef CONFIG_SEC_PM
	wake_lock_destroy(&resume_wakelock);
#endif
}
#else
static void qpnp_rtc_shutdown(struct spmi_device *spmi)
{
	u8 value[4] = {0};
	u8 reg;
	int rc;
	unsigned long irq_flags;
	struct qpnp_rtc *rtc_dd = dev_get_drvdata(&spmi->dev);
	bool rtc_alarm_powerup = rtc_dd->rtc_alarm_powerup;

	if (!rtc_alarm_powerup && !poweron_alarm) {
		spin_lock_irqsave(&rtc_dd->alarm_ctrl_lock, irq_flags);
		dev_dbg(&spmi->dev, "Disabling alarm interrupts\n");

		/* Disable RTC alarms */
		reg = rtc_dd->alarm_ctrl_reg1;
		reg &= ~BIT_RTC_ALARM_ENABLE;
		rc = qpnp_write_wrapper(rtc_dd, &reg,
			rtc_dd->alarm_base + REG_OFFSET_ALARM_CTRL1, 1);
		if (rc) {
			dev_err(rtc_dd->rtc_dev, "SPMI write failed\n");
			goto fail_alarm_disable;
		}

		/* Clear Alarm register */
		rc = qpnp_write_wrapper(rtc_dd, value,
				rtc_dd->alarm_base + REG_OFFSET_ALARM_RW,
				NUM_8_BIT_RTC_REGS);
		if (rc)
			dev_err(rtc_dd->rtc_dev, "SPMI write failed\n");

fail_alarm_disable:
		spin_unlock_irqrestore(&rtc_dd->alarm_ctrl_lock, irq_flags);
	}

#ifdef CONFIG_SEC_PM
	wake_lock_destroy(&resume_wakelock);
#endif
}
#endif /* CONFIG_RTC_AUTO_PWRON */

static struct of_device_id spmi_match_table[] = {
	{
		.compatible = "qcom,qpnp-rtc",
	},
	{}
};

static struct spmi_driver qpnp_rtc_driver = {
	.probe          = qpnp_rtc_probe,
	.remove         = __devexit_p(qpnp_rtc_remove),
	.shutdown       = qpnp_rtc_shutdown,
	.driver = {
		.name   = "qcom,qpnp-rtc",
		.owner  = THIS_MODULE,
		.of_match_table = spmi_match_table,
#ifdef CONFIG_SEC_PM
		.pm	= &qpnp_rtc_pm_ops,
#endif
	},
};

static int __init qpnp_rtc_init(void)
{
	return spmi_driver_register(&qpnp_rtc_driver);
}
module_init(qpnp_rtc_init);

static void __exit qpnp_rtc_exit(void)
{
	spmi_driver_unregister(&qpnp_rtc_driver);
}
module_exit(qpnp_rtc_exit);

MODULE_DESCRIPTION("SMPI PMIC RTC driver");
MODULE_LICENSE("GPL V2");
