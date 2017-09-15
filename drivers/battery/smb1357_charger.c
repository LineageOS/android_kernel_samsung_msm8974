/*
 *  smb1357_charger.c
 *  Samsung SMB1357 Charger Driver
 *
 *  Copyright (C) 2014 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#define DEBUG

#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_charger.h>
#include <linux/battery/charger/smb1357_charger.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/qpnp/pin.h>
#include <linux/qpnp/qpnp-adc.h>
#ifdef CONFIG_USB_HOST_NOTIFY
#include <linux/host_notify.h>
#endif
#include <mach/sec_debug.h>

#define	SIOP_INPUT_LIMIT_CURRENT 1200
#define	SIOP_CHARGING_LIMIT_CURRENT 1000
#define CHG_THERM_REDUCE_MAX_TEMP 650
#define CHG_THERM_INPUT_LIMIT_CURRENT 1800
#define CHG_THERM_CHARGING_LIMIT_CURRENT 2400

#define SECOND_TERMINATION_CURRENT 4

#if defined(CONFIG_ARCH_MSM8974PRO)
#define CHG_THEM_THEM_CHANNEL LR_MUX8_PU1_AMUX_THM4
#else
#define CHG_THEM_THEM_CHANNEL LR_MUX8_PU2_AMUX_THM4
#endif

#if defined(CONFIG_MACH_KLTE_JPN) || defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_CHAGALL_KDI)
#define DT_NODE_NAME	"charger"
#define DEV_CHG_NAME	"sec-charger"
#define ENABLE_MAX77804K_CHG_IRQ
#define SMB_OTG_NOTIFY_OVERCURRENT
#define OVP_MSM_GPIO_CONTROL
#endif

#if defined(CONFIG_SEC_TRLTE_JPN) || defined(CONFIG_SEC_TBLTE_JPN)
#define DT_NODE_NAME	"smb1357charger"
#define DEV_CHG_NAME	"smb1357-charger"
#define ENABLE_EXTERNAL_APSD
#define POWER_SUPPLY_TYPE_POGODOCK	POWER_SUPPLY_TYPE_MAX
#endif

#if defined(CONFIG_MACH_KACTIVELTE_DCM)
#define ENABLE_SHUTDOWN_MODE
#endif

#if !defined(CONFIG_MACH_CHAGALL_KDI)
#define ENABLE_CHG_THERM_LIMIT
#endif

#define REDUCE_TMM_CHG
//#define ENABLE_CHG_THERM_LIMIT
//#define ENABLE_DCIN_5VOR9V
//#define ENABLE_SYSON_TRIGGER
//#define ENABLE_SMBCHG_BATT_DET
//#define ENABLE_SHUTDOWN_MODE
//#define USE_SYSFS_CURRENT_CTRL
//#define TIMER_FORCE_BLOCK
//#define CHECK_VF_BY_IRQ
//#define USE_DEBUG_WORK

#define	CS21_REVISION	0x25

#if defined(REDUCE_TMM_CHG)
#define TMM_SIOP_LVL 3
#define TMM_CHG_CURRENT 300
#endif

#if defined(TIMER_FORCE_BLOCK)
#define FORCE_BLOCK_MAX	(10 * 60 / 30)
#define FORCE_BLOCK_MIN (5 * 60 / 30)
static int force_block;
static int force_block_count;
#endif

extern int sec_otg_notify(int event);
extern unsigned int sec_dbg_level;

#define smb_dbg(dev, fmt, arg...)	\
	do {							\
		if (sec_dbg_level != KERNEL_SEC_DEBUG_LEVEL_LOW)	\
			dev_info(dev, fmt, ##arg);	\
	} while(0)

#define smb_info(dev, fmt, arg...)	\
	do {							\
		dev_info(dev, fmt, ##arg);	\
	} while(0)


struct smb1357_charger_data{
	struct power_supply *psy_bat;
	struct power_supply psy_pogo;
	struct sec_charger_info *charger;
	struct delayed_work init_work;
	struct delayed_work debug_work;
	struct delayed_work hvdcp_det_work;
	struct delayed_work pogo_det_work;
	struct delayed_work syson_trigger_work;
	struct delayed_work detbat_work;
	struct regulator *chg_vf_1p8v;
	struct wake_lock chg_wake_lock;
	struct wake_lock chgisr_wake_lock;
	struct mutex mutex;
	u8 revision;
	u8 hvdcp_det_count;
	u8 pogo_det_count;
	u8 pogo_status;
	u8 syson_trigger_onoff;
	int siop_level;
	int pogo_det_gpio;
	int shdn_gpio;
	int chg_limit;
	int chg_high_temp;
	int chg_high_temp_recovery;
	int ovp_gpio_en;
	int chg_vf_det;
	int detbat_irq;
	int usbin_cable_type;
	u8 pogo_2nd_charging;
	u8 sysfs_mode;
	u32 sysfs_curin;
	u32 sysfs_curout;
};

#if defined(ENABLE_MAX77804K_CHG_IRQ)
static int max77804k_tiny_irq_batp;

struct max77804k_tiny_charger_data{
	struct max77804k_dev *max77804k;
	struct delayed_work	 tiny_init_work;
};
#endif

static struct qpnp_pin_cfg smb1357_pm_mpp_digin_config = {
	.mode = 0,			/* QPNP_PIN_MODE_DIG_IN */
	.output_type = 0,	/* QPNP_PIN_OUT_BUF_CMOS */
	.invert = 0,		/* QPNP_PIN_INVERT_DISABLE */
	.pull = 1,			/* PM8XXX_MPP_BI_PULLUP_OPEN */
	.vin_sel = 2,
	.out_strength = 3,	/* QPNP_PIN_OUT_STRENGTH_HIGH */
	.src_sel = 0,		/* QPNP_PIN_SEL_FUNC_CONSTANT */
	.master_en = 1,		/* QPNP_PIN_MASTER_ENABLE */

};

static struct qpnp_pin_cfg smb1357_pm_mpp_ain_config = {
	.mode = 4,			/* QPNP_PIN_MODE_AIN */
	.output_type = 0,	/* QPNP_PIN_OUT_BUF_CMOS */
	.invert = 0,		/* QPNP_PIN_INVERT_DISABLE */
	.pull = 1,			/* PM8XXX_MPP_BI_PULLUP_OPEN */
	.vin_sel = 2,
	.ain_route = 3,		/*  QPNP_PIN_AIN_AMUX_CH8 */
	.out_strength = 3,	/* QPNP_PIN_OUT_STRENGTH_HIGH */
	.src_sel = 0,		/* QPNP_PIN_SEL_FUNC_CONSTANT */
	.master_en = 1,		/* QPNP_PIN_MASTER_ENABLE */

};

static enum power_supply_property pogo_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *pogo_supply_list[] = {
	"battery",
};

static int chg_fast_cur_table[] = {
	300,	// 0x00
	400,
	450,
	475,
	500,
	550,
	600,
	650,
	700,	// 0x08
	900,
	950,
	1000,
	1100,
	1200,
	1400,
	1450,
	1500,	// 0x10
	1600,
	1800,
	1850,
	1880,
	1910,
	1930,
	1950,
	1970,	// 0x18
	2000,
	2050,
	2100,
	2300,
	2400,
	2500,
	3000	// 0x1F
};
static int chg_pre_cur_table[] = {
	100,
	150,
	200,
	250
};

static u32 chg_temp_table_size;
const static sec_bat_adc_table_data_t chg_temp_table[] = {
	{25844, 900},
	{26113, 850},
	{26595, 800},
	{26896, 750},
	{27238, 700},
	{27679, 650},
	{28193, 600},
	{28768, 550},
	{29412, 500},
	{30169, 450},
	{31008, 400},
	{31917, 350},
	{32901, 300},
	{33955, 250},
	{35039, 200},
	{36132, 150},
	{37183, 100},
	{38158, 50},
	{39041, 0},
	{39856, -50},
	{40561, -100},
	{41146, -150},
	{41622, -200},
};

enum {
	CUR_MODE = 6,
	CUR_SET,
	USE_SYSFS_CUR = 0xff,
};

static void ovp_gpio_enable(int gpio, int value)
{
#if defined(OVP_MSM_GPIO_CONTROL)
	// msmgpio.
	gpio_set_value(gpio, value);
#else
	// gpio expander.
	gpio_direction_output(gpio, value);
#endif
	return;
}

static int smb1357_charger_i2c_write(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, *buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)buf %d\n", __func__, ret,*buf);

	return ret;
}

static int smb1357_charger_i2c_read(struct i2c_client *client,
				int reg, u8 *buf)
{
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, reg, 1, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: Error(%d)\n", __func__, ret);

	return ret;
}

static int smb1357_charger_masked_write_reg(struct i2c_client *client,
				u8 reg, u8 mask, u8 val)
{
	int ret;
	u8 buf;

	ret = smb1357_charger_i2c_read(client, reg, &buf);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - read(%d)\n", __func__, ret);
		goto error;
	}

	buf &= ~mask;
	buf |= val & mask;

	ret = smb1357_charger_i2c_write(client, reg, &buf);
	if (ret < 0) {
		dev_err(&client->dev, "%s: error - write(%d)\n", __func__, ret);
		goto error;
	}

error:
	return ret;
}

static void smb1357_read_regs(struct i2c_client *client, char *str)
{
	u8 data = 0;
	u32 addr = 0;

	for (addr = 0; addr <= 0x1f; addr++) {
		smb1357_charger_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}

	/* "#" considered as new line in application */
	sprintf(str+strlen(str), "#");

	for (addr = 0x40; addr <= 0x57; addr++) {
		smb1357_charger_i2c_read(client, addr, &data);
		sprintf(str+strlen(str), "0x%x, ", data);
	}
}

static u8  smb1357_check_product_id(struct i2c_client * client)
{
	struct smb1357_charger_data *smb1357data =
				dev_get_drvdata(&client->dev);
	u8 id = 0x00;
	int ret;

	smb1357_charger_i2c_read(client, STATUS_9_REG, &id);
	smb1357data->revision = id;
	dev_dbg(&client->dev,"%s : Revision (0x%2x)\n", __func__, id);

	ret = smb1357_charger_i2c_read(client, SMB1357_PRODUCT_ID, &id);

	if (ret < 0) {
		dev_err(&client->dev,"%s : ID Read FAILED..[%d]\n",
			__func__, ret);
		id = 0xFF;
	}
	else {
		dev_info(&client->dev,"%s : charger ID %s\n",
			__func__, id ? "SMB1356" : "SMB1357");
	}

	return id;
}

static void smb1357_write_enable(struct i2c_client * client, u8 enable)
{
	u8 buf;
	if (enable == true)	buf = ALLOW_VOLATILE_BIT;
	else				buf = 0x00;

	smb1357_charger_masked_write_reg(client, CMD_I2C_REG,
			ALLOW_VOLATILE_BIT, buf);

	dev_dbg(&client->dev,"%s : %s\n", __func__, enable ? "EN":"DISABLE");

	return;
}

static void smb1357_set_APSD(struct i2c_client *client, u8 enable)
{
	msleep(20);
	smb1357_charger_masked_write_reg(client, CFG_11_REG,
			AUTO_DET_SRC_EN_BIT, AUTO_DET_SRC_EN_BIT);

	if (enable) {
		smb1357_charger_masked_write_reg(client, CMD_INPUT_LIMIT,
			USE_REGISTER_FOR_CURRENT, 0x00);
	}
	else {
		smb1357_charger_masked_write_reg(client, CMD_INPUT_LIMIT,
			USE_REGISTER_FOR_CURRENT, USE_REGISTER_FOR_CURRENT);
	}
	dev_dbg(&client->dev,"%s : %s\n",
				__func__, enable ? "EN":"DISABLE");

	return;
}

static int smb1357_check_vf_status(struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&client->dev);
	int vf_removed = 0;
	int ret;

	if (smb1357data->chg_vf_det){
		if (regulator_enable(smb1357data->chg_vf_1p8v)) {
			msleep(10);
			qpnp_pin_config(smb1357data->chg_vf_det,
					&smb1357_pm_mpp_digin_config);

			/* VF_DEF is HIGH - batt removed. */
			vf_removed = gpio_get_value(smb1357data->chg_vf_det);

			qpnp_pin_config(smb1357data->chg_vf_det, &smb1357_pm_mpp_ain_config);
			regulator_disable(smb1357data->chg_vf_1p8v);
			msleep(10);
		}
	}

	if (vf_removed == 1)	ret = false;
	else					ret = true;

	dev_info(&client->dev, "%s batt status %s\n",
		__func__, ret ? "OK":"Removed");
	return ret;
}

static int smb1357_cvt_adc_to_temp(struct i2c_client *client, int adc)
{
	int temp = 0;
	int high = 0;
	int mid = 0;
	int low = 0;

	/* check min,max temp */
	if (chg_temp_table[0].adc >= adc){
		temp = chg_temp_table[0].data;
		goto out;
	}
	else if (chg_temp_table[chg_temp_table_size-1].adc <= adc){
		temp = chg_temp_table[chg_temp_table_size-1].data;
		goto out;
	}

	high = chg_temp_table_size - 1;

	while (low <= high) {
		mid = (low + high) / 2;
		if (chg_temp_table[mid].adc > adc)
			high = mid - 1;
		else if (chg_temp_table[mid].adc < adc)
			low = mid + 1;
		else {
			temp = chg_temp_table[mid].data;
			goto out;
		}
	}

	temp = chg_temp_table[high].data;
	temp += ((chg_temp_table[low].data - chg_temp_table[high].data) *
		(adc - chg_temp_table[high].adc)) /
		(chg_temp_table[low].adc - chg_temp_table[high].adc);

out:
	dev_info(&client->dev,"%s adc [%d] temp [ %3d]\n",
					__func__, adc, temp);
	return temp;
}

static int smb1357_get_therm_temp(struct i2c_client *client)
{
	struct qpnp_vadc_result results;
	int temp = 0;
	int err = 0;

	err = qpnp_vadc_read(NULL, CHG_THEM_THEM_CHANNEL, &results);

	if (err < 0) {
		dev_err(&client->dev,"%s : therm read fail rc = %d\n",
			__func__, err);

		temp = 300;
	}
	else {
		temp = smb1357_cvt_adc_to_temp(client, results.adc_code);
	}

	return temp;
}

static void smb1357_set_STAT_conf(struct i2c_client *client)
{
	u8 buf;

	smb1357_charger_masked_write_reg(client, CFG_17_REG,
		CHG_STAT_DISABLE_BIT | \
		CHG_STAT_ACTIVE_HIGH_BIT | \
		CHG_STAT_SRC_BIT | \
		CHG_STAT_IRQ_ONLY_BIT, 0x00);

	smb1357_charger_i2c_read(client, CFG_17_REG, &buf);

	dev_dbg(&client->dev,"%s : default [0x%x]\n",
					__func__, buf);
	return;
}

static void smb1357_set_VSYS(struct i2c_client *client)
{
	/* min vsys voltage : 3600 (0x20) */
	smb1357_charger_masked_write_reg(client,CFG_4_REG,
		CHG_MIN_VSYS_MASK, 0x20);

	dev_dbg(&client->dev,"%s\n",__func__);
	return;
}

static void smb1357_lowbatt_threshold(struct i2c_client *client)
{
	/* low batt threshold : 3250 (0x0c) */
	smb1357_charger_masked_write_reg(client, CFG_13_REG,
		LOW_BATT_THRESHOLD_MASK, 0x0C);

	dev_dbg(&client->dev,"%s\n",__func__);
	return;
}

static void smb1357_usb_current_limit(struct i2c_client *client,
		u8 usb_type, u8 usb_mode)
{
	/* otg batt under voltage : 3100 */
	smb1357_charger_masked_write_reg(client, CFG_4_REG,
		OTG_BATT_UV_MASK , 0x08);

	/* USB TYPE : 0 - USB2.0 : 1 - USB3.0 */
	smb1357_charger_masked_write_reg(client, CFG_5_REG,
		USB_2_3_BIT, usb_type);

	/* USB Mode : 0 - USB500 : 1 - USBAC : 2 - USB100 */
	smb1357_charger_masked_write_reg(client, CMD_INPUT_LIMIT,
		USB_100_500_AC_MASK, usb_mode);

	dev_info(&client->dev,"%s type : %d mode %d\n",
			__func__, usb_type, usb_mode);
	return;
}

static void smb1357_temp_compensation(struct i2c_client *client)
{
	/* hot/cold soft vfloat comp disable. */
	smb1357_charger_masked_write_reg(client, CFG_1A_REG,
		HOT_SOFT_VFLOAT_COMP_EN_BIT | COLD_SOFT_VFLOAT_COMP_EN_BIT,
		0x00);

	dev_dbg(&client->dev,"%s\n",__func__);
	return;
}

static void smb1357_dcin_adapter(struct i2c_client *client, u8 data)
{
#ifdef CONFIG_SEC_FACTORY
	data = DCIN_5VOR9V;
#endif
	/* DCIN ADAPTER : 5V or(to) 9V */
	smb1357_charger_masked_write_reg(client, CFG_A_REG,
		DCIN_ADAPTER_MASK, data);
	msleep(50);

	dev_dbg(&client->dev,"%s, data = 0x%x\n",__func__, data);
	return;
}

static void smb1357_syson_trigger_work(struct work_struct *work)
{
	struct smb1357_charger_data *smb1357data =
	    container_of(work, struct smb1357_charger_data,syson_trigger_work.work);
	struct sec_charger_info *charger = smb1357data->charger;
	struct i2c_client *client = charger->client;

	cancel_delayed_work(&smb1357data->syson_trigger_work);
#ifdef ENABLE_SYSON_TRIGGER
	// TODO. En/Disable VBUS sensing.
	if (true) {
		extern void dwc3_msm_config_vbus_sensing(bool enable);
		dwc3_msm_config_vbus_sensing(smb1357data->syson_trigger_onoff);
	}
#endif
	dev_dbg(&client->dev,"%s, on = [%d]\n",
			__func__,smb1357data->syson_trigger_onoff);
	return;
}

static void smb1357_syson_sensing(struct i2c_client *client,
				u32 delay, bool onoff)
{
	struct smb1357_charger_data *smb1357data =
						i2c_get_clientdata(client);

	smb1357data->syson_trigger_onoff = onoff;
#ifdef ENABLE_SYSON_TRIGGER
	cancel_delayed_work(&smb1357data->syson_trigger_work);
	schedule_delayed_work(&smb1357data->syson_trigger_work,
		msecs_to_jiffies(delay));
#endif
	dev_dbg(&client->dev,"%s, delay[%d] on = [%d]\n",
		__func__, delay, onoff);
	return;
}

static void smb1357_dcin_config_hv_lv(struct i2c_client *client, u8 mode)
{
	u8 data_e = 0xff;

#ifdef ENABLE_DCIN_5VOR9V
	if (mode) {
		smb1357_dcin_adapter(client, DCIN_5VOR9V);
		msleep(10);
	}

	smb1357_charger_i2c_read(client, IRQ_E_REG, &data_e);
	if (data_e & IRQ_E_DC_OV_BIT) {
		/* dcin 9v only */
		smb1357_dcin_adapter(client, DCIN_9VONLY);
		smb1357_syson_sensing(client, 100, false);
	}
	else if (data_e & IRQ_E_DC_UV_BIT) {
		/* dcin 5v or 9v */
		smb1357_dcin_adapter(client, DCIN_5VOR9V);
	}
#else
	smb1357_dcin_adapter(client, DCIN_5VONLY);
#endif
	msleep(10);
	dev_info(&client->dev,"%s, dcin mode %d, OV_UV 0x%x\n",
			__func__, mode, data_e);
	return;
}

static u8 smb1357_dcin_input_voltage(struct i2c_client *client)
{
	u8 status_8;
	u8 ret;

	smb1357_charger_i2c_read(client, STATUS_8_REG, &status_8);

	if (status_8 & DCIN_HV)
		ret = DCIN_9V;
	else if (status_8 & DCIN_LV)
		ret = DCIN_5V;
	else
		ret = DCIN_NONE;

	return ret;
}

static void smb1357_clear_irq_status(struct i2c_client *client)
{
	u8 buf;
	u8 reg = IRQ_A_REG;

	do{
		smb1357_charger_i2c_read(client, reg, &buf);
	}while(++reg <= IRQ_G_REG);

	dev_dbg(&client->dev,"%s\n",__func__);
	return;
}

static void smb1357_enable_irq(struct i2c_client *client,
		u8 reg, u8 mask, u8 buf)
{
	if(reg < IRQ_CFG_REG && reg > IRQ3_CFG_REG)
	{
		dev_dbg(&client->dev,"%s, reg error 0x%2x\n",__func__,reg);
		goto out;
	}

	smb1357_charger_masked_write_reg(client, reg, mask, buf);
	dev_dbg(&client->dev,"%s, reg 0x%2x\n",__func__,reg);
out:
	return;
}

static void smb1357_enable_charging(struct i2c_client *client, u8 enable)
{
	u8 buf;

	if(enable == true)	buf = CMD_CHG_EN_VAL;
	else				buf = CMD_CHG_DIS_VAL;

	smb1357_charger_masked_write_reg(client, CMD_CHG_REG,
		CMD_CHG_EN_MASK, buf);

	dev_info(&client->dev,"%s, enable = %d\n",__func__, enable);
	return;
}

static void smb1357_enable_otg(struct i2c_client *client, u8 enable)
{
	smb1357_charger_masked_write_reg(client, CMD_CHG_REG,
		OTG_EN, enable);

	dev_info(&client->dev,"%s, enable = %d\n",__func__, enable);
	return;
}

static void smb1357_chgin_shutdown(struct i2c_client *client, u8 enable)
{
	if (enable == true){
		smb1357_charger_masked_write_reg(client, CMD_INPUT_LIMIT,
			USB_SHUTDOWN_BIT|DC_SHUTDOWN_BIT,
			USB_SHUTDOWN_BIT|DC_SHUTDOWN_BIT);
	}
	else {
		smb1357_charger_masked_write_reg(client, CMD_INPUT_LIMIT,
			USB_SHUTDOWN_BIT|DC_SHUTDOWN_BIT,
			0x00);
	}

	dev_dbg(&client->dev,"%s, enable = %d\n",__func__, enable);
	return;
}

static void smb1357_enable_batt_irq(struct i2c_client *client, u8 enable)
{
	if (enable == true){
	/* enable battery missing algoritm */
	smb1357_charger_masked_write_reg(client, CFG_19_REG,
		BATT_MISSING_ALGO_BIT|BATT_MISSING_BMD_BIT | \
		BATT_MISSING_PLUG_BIT|BATT_MISSING_POLLING_BIT,
		BATT_MISSING_ALGO_BIT|BATT_MISSING_BMD_BIT | \
		BATT_MISSING_PLUG_BIT|BATT_MISSING_POLLING_BIT);
	}
	else
	{
	/* disable battery missing algoritm */
	smb1357_charger_masked_write_reg(client, CFG_19_REG,
		BATT_MISSING_ALGO_BIT|BATT_MISSING_BMD_BIT | \
		BATT_MISSING_PLUG_BIT|BATT_MISSING_POLLING_BIT,
		0x00);
	}
	/* set battery irq */
	smb1357_charger_masked_write_reg(client, IRQ2_CFG_REG,
		(IRQ2_BATT_MISSING_BIT|IRQ2_VBAT_LOW_BIT),
		(enable << 1 | enable));

	dev_dbg(&client->dev,"%s, enable = %d\n",__func__, enable);
	return;
}

#if defined(ENABLE_SMBCHG_BATT_DET)
static u8 smb1357_get_batt_status(struct i2c_client *client)
{
	u8 data;

	smb1357_charger_i2c_read(client, IRQ_B_REG, &data);
	dev_dbg(&client->dev,
		"%s: irq B [ 0x%x ]\n", __func__, data);

	/* false : battery missing, true : battery using */
	if ((data & IRQ_B_BATT_TERMINAL_BIT)||
		(data & IRQ_B_BATT_MISSING_BIT)||
		(data & IRQ_B_VBAT_LOW_BIT)) {
			data = false;
			dev_dbg(&client->dev,"%s, batt = %s\n",
				__func__, data ? "using":"missing");
	}
	else
		data = true;

	return data;
}
#endif

static u8 smb1357_get_charger_type(struct i2c_client *client)
{
	u8 hvdcp_type;
	u8 chg_type;
	u8 port;

	smb1357_charger_i2c_read(client, STATUS_5_REG, &chg_type);
	dev_dbg(&client->dev,"%s, chgtype = 0x%x\n", __func__, chg_type);

	if (chg_type & CDP_BIT) {
		port = CDP;
	}
	else if (chg_type & DCP_BIT) {
		smb1357_charger_i2c_read(client, STATUS_7_REG, &hvdcp_type);

		if (hvdcp_type & HVDCP_SEL_12V_BIT)
			port = HVDCP_12V;
		else if (hvdcp_type & HVDCP_SEL_9V_BIT)
			port = HVDCP_9V;
		else if (hvdcp_type & HVDCP_SEL_5V_BIT)
			port = HVDCP_5V;
		else
			port = DCP;

		dev_dbg(&client->dev,"%s, hvdcp = %d\n", __func__, hvdcp_type);
	}
	else if (chg_type & OTHER_BIT) {
		port = OCP;
	}
	else if (chg_type & SDP_BIT) {
		port = SDP;
	}
	else
		port = SDP;

	dev_info(&client->dev,"%s, port = %d\n", __func__,  port);
	return port;
}


static u8 smb1357_get_float_voltage_data(
			int float_voltage)
{
	u8 data;

	if (float_voltage < 3500)
		data = 0;
	else if(float_voltage <= 4340)
		data = (float_voltage - 3500) / 20;
	else if(float_voltage == 4350)
		data = 0x2B; /* (4340 -3500)/20 + 1 */
	else if(float_voltage <= 4500)
		data = (float_voltage - 3500) / 20 + 1;
	else
		data = 0x2B;

	return data;
}
static void smb1357_set_vfloat(struct i2c_client *client,int f_vol)
{
	u8 data;

	data = smb1357_get_float_voltage_data(f_vol);

	smb1357_charger_masked_write_reg(client, VFLOAT_REG,
		FLOAT_MASK, data);

	return;
}
static u8 smb1357_get_current_data(struct i2c_client *client, int cur)
{
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&client->dev);
	u8 data = 0;
	int max_reg = (int)(sizeof(chg_fast_cur_table)/sizeof(int));

	do{
		if (cur <= chg_fast_cur_table[data]){
			break;
		}
	}while(++data <= max_reg);

	if (smb1357data->revision >= CS21_REVISION) {
		/* check register value */
		if ((data == 0x0F) || (data == 0x16))
			data -= 0x01;

		/* fixed current on cs21 sample */
		if (cur == 2700)	data = 0x0F;
		else if (cur == 2800)	data = 0x16;
	}

	if(data >= max_reg)	data = 0x00;

	dev_dbg(&client->dev,"%s : current [%d]: reg [0x%x]!\n",
			__func__, cur, data);
	return data;
}

static u8 smb1357_get_term_current_limit_data(
			int termination_current)
{
	u8 data;

	if (termination_current <= 50)
		data = 0x01;
	else if (termination_current <= 100)
		data = 0x02;
	else if (termination_current <= 150)
		data = 0x03;
	else if (termination_current <= 200)
		data = 0x04;
	else if (termination_current <= 250)
		data = 0x05;
	else if (termination_current <= 300)
		data = 0x00;
	else if (termination_current <= 500)
		data = 0x06;
	else if (termination_current <= 600)
		data = 0x07;
	else
		data = 0x07;	/* set input current limit as maximum */

	data = data << 3;

	return data;
}

static void smb1357_set_cur_termination(struct i2c_client *client, u8 enable)
{
	u8 data;
	if(enable == true)	data = 0x00;
	else				data = 0x08;

	smb1357_charger_masked_write_reg(client, CFG_14_REG,
		DISABLE_CURRENT_TERM_BIT, data);

	dev_dbg(&client->dev,"%s : %s\n",
			__func__, enable ? "EN":"DISABLE");

	return;

}

static void smb1357_set_auto_recharge(struct i2c_client *client, u8 enable)
{
	u8 data;
	if(enable == true)	data = 0x00;
	else				data = 0x04;

	/* recharge voltage threshold : VFLT - 100mv */
	smb1357_charger_masked_write_reg(client, CFG_5_REG,
		RECHARGE_200MV_BIT, 0x00);

	/* automatic recharge */
	smb1357_charger_masked_write_reg(client, CFG_14_REG,
		DISABLE_AUTO_RECHARGE_BIT, data);

	dev_dbg(&client->dev,"%s : %s\n",
				__func__, enable ? "EN":"DISABLE");
	return;
}

static void smb1357_set_charging_current(
		struct i2c_client *client, int charging_current)
{
	u8 data;

	data = smb1357_get_current_data(client, charging_current);

	smb1357_charger_masked_write_reg(client, CFG_1C_REG,
		FAST_CHARGE_CURRENT_MASK, data);

	dev_info(&client->dev,"%s : chg_cur = [%4d]data :(0x%02x)\n",
		__func__, charging_current, data);

	return;
}

static void smb1357_set_input_current_limit(
		struct i2c_client *client, int input_current_limit)
{
	u8 data;

	/* disable AICL */
	smb1357_charger_masked_write_reg(client, CFG_D_REG,
		USBIN_AICL_BIT, 0x00);
	smb1357_charger_masked_write_reg(client, USBIN_DCIN_CFG_REG,
		AICL_5V_THRESHOLD_BIT, AICL_5V_THRESHOLD_BIT);

	/* Input current limit */
	data = smb1357_get_current_data(client, input_current_limit);

	smb1357_charger_masked_write_reg(client, CFG_C_REG,
		USBIN_INPUT_MASK, data);

	smb1357_charger_masked_write_reg(client, CFG_A_REG,
		DCIN_INPUT_MASK, data);

	msleep(50);
	/* enable AICL */
	smb1357_charger_masked_write_reg(client, CFG_D_REG,
		USBIN_AICL_BIT, USBIN_AICL_BIT);

	dev_dbg(&client->dev,
		"%s : data : (0x%02x)\n", __func__, data);

	return;
}

static int smb1357_status_charging_current(	struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&client->dev);
	u8 data = 0;
	u8 status = 0;
	u32 cur = 0;

	smb1357_charger_i2c_read(client,STATUS_3_REG, &data);
	smb1357_charger_i2c_read(client,STATUS_4_REG, &status);

	if (status & BATT_DONE_STATUS)
		status = BATT_FAST_CHG_VAL;
	else
		status = (status & CHG_TYPE_MASK) >> 1;

	switch(status){
		case BATT_PRE_CHG_VAL:
			data &= (data&PRE_CHG_CUR_COMP_MASK) >> 5;
			cur = chg_pre_cur_table[data];
			break;
		case BATT_FAST_CHG_VAL:
		case BATT_TAPER_CHG_VAL:
			data &= FAST_CHG_CUR_COMP_MASK;
			cur = chg_fast_cur_table[data];

			if (smb1357data->revision >= CS21_REVISION) {
				if (data == 0x0F)	cur = 2700;
				else if (data == 0x16)	cur = 2800;
			}
			break;
		case BATT_NOT_CHG_VAL:
		default:
			cur = 0;
			break;
	}

	dev_info(&client->dev,
		"%s : data : (0x%02x), cur = [%d] status = [0x%x]\n",
		__func__, data, cur, status);

	return cur;
}

static void smb1357_irq_cfg(struct i2c_client * client)
{
	smb1357_enable_irq(client, IRQ_CFG_REG, 0x0F,
					IRQ_USBIN_UV_BIT);
	smb1357_enable_irq(client, IRQ2_CFG_REG, 0xFF,
					IRQ2_POWER_OK_BIT);
	smb1357_enable_irq(client, IRQ3_CFG_REG, 0xFF,
					IRQ3_DCIN_OV_BIT|IRQ3_DCIN_UV_BIT);
}

static void smb1357_hw_init(struct i2c_client * client)
{
	/* move to bootlaoder for lowbatt booting */
	/* APSD disable */
	/* smb1357_set_APSD(client, false); */
	/* USBIN ADAPTER : 9V only */
	/* smb1357_charger_masked_write_reg(client, CFG_C_REG, */
	/* USBIN_ADAPTER_MASK, 0x60); */

	/* DCIN ADAPTER : 5V to(or) 9V */
	smb1357_dcin_config_hv_lv(client, true);

	/* DCIN Disable AICL */
	smb1357_charger_masked_write_reg(client, CFG_B_REG,
		DCIN_AICL_BIT, 0x00);

	/* USBIN Input Current : 1800mA */
	smb1357_set_input_current_limit(client, 1800);

	/* PRE CHG CUR : 250mA */
	smb1357_charger_masked_write_reg(client, CFG_1C_REG,
		PRE_CHG_CUR_MASK, 0x60);

	/* Fast CHG CUR : 2500mA */
	smb1357_set_charging_current(client, 2500);

	/* irq active low */
	smb1357_set_STAT_conf(client);

	/* Config Default irq */
	smb1357_irq_cfg(client);

	/* Clear Irq */
	smb1357_clear_irq_status(client);

	/* Enable batt irq */
	smb1357_enable_batt_irq(client, false);

	dev_info(&client->dev,"%s\n",__func__);
	return;
}

static void smb1357_set_default_data(struct i2c_client *client)
{
	u8 data;
	u8 data_f;

	/* Set the registers to the default configuration */
	smb1357_charger_i2c_read(client, IRQ_F_REG, &data_f);
	if ((data_f & IRQ_F_POWER_OK_BIT) == false) {
		/* Sometimes It didn't detcted TA_HV */
		/* USBIN ADAPTER : 9V */
		smb1357_charger_masked_write_reg(client, CFG_C_REG,
			USBIN_ADAPTER_MASK, 0x60);
		msleep(20);

		/* USBIN ADAPTER : 5v or 9v */
		smb1357_charger_masked_write_reg(client, CFG_C_REG,
			USBIN_ADAPTER_MASK, 0x20);

		/* APSD */
		smb1357_set_APSD(client, false);
	}

	/* Disable Automatic Recharge */
	smb1357_set_auto_recharge(client, false);

	/* usb mode */
	smb1357_usb_current_limit(client, USB3_VAL, USB_500_VAL);

	/* AICL */
	data = 0x04;
	smb1357_charger_i2c_write(client, CFG_D_REG, &data);

	/* DCIN Disable AICL */
	smb1357_charger_masked_write_reg(client, CFG_B_REG,
		DCIN_AICL_BIT, 0x00);

	/* DCIN 5V to(or) 9V */
	smb1357_dcin_config_hv_lv(client, true);

	/* config irq */
	smb1357_irq_cfg(client);

	/* Clear Irq */
	smb1357_clear_irq_status(client);

	dev_dbg(&client->dev,"%s, PA 0x%x\n", __func__, data_f);
	return;
}

static int smb1357_get_charging_status(struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data =
						i2c_get_clientdata(client);
	int status = POWER_SUPPLY_STATUS_UNKNOWN;
	u8 status_1 = 0;
	u8 status_4 = 0;

	/* need delay to update charger status */
	msleep(10);

	smb1357_charger_i2c_read(client, STATUS_1_REG, &status_1);
	smb1357_charger_i2c_read(client, STATUS_4_REG, &status_4);

	dev_info(&client->dev,"chg status:s1[0x%2x],s4[0x%2x]\n",
		status_1, status_4);

	/* At least one charge cycle terminated,
	 * Charge current < Termination Current
	 */
	if ((smb1357data->hvdcp_det_count == 0) &&
		(status_4 & BATT_DONE_STATUS)) {
		/* top-off by full charging */
		status = POWER_SUPPLY_STATUS_FULL;
		goto charging_status_end;
	}

	/* Is Charge enabled ? */
	if (status_4 & CHG_EN_BIT) {
		/* CHG Status [2:1] : no Chg (0b00)
		 * not charging
		 */
		if (!(status_4 & CHG_TYPE_MASK)) {
			status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			goto charging_status_end;
		} else {
			status = POWER_SUPPLY_STATUS_CHARGING;
			goto charging_status_end;
		}
	} else
		status = POWER_SUPPLY_STATUS_DISCHARGING;

charging_status_end:
	return (int)status;
}

static int smb1357_get_charging_health(struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&client->dev);
	struct sec_charger_info *charger = smb1357data->charger;
	int health = POWER_SUPPLY_HEALTH_GOOD;
	u8 status_1;
	u8 status_4;
	u8 status_8;
	u8 irq_e;
	u8 chg_en;
	u8 chg_cur;
	u8 gpio = false;

	smb1357_charger_i2c_read(client, STATUS_1_REG, &status_1);
	smb1357_charger_i2c_read(client, STATUS_4_REG, &status_4);
	smb1357_charger_i2c_read(client, STATUS_8_REG, &status_8);
	smb1357_charger_i2c_read(client, IRQ_E_REG, &irq_e);
	smb1357_charger_i2c_read(client, CMD_CHG_REG, &chg_en);
	smb1357_charger_i2c_read(client, CFG_1C_REG, &chg_cur);

	if (smb1357data->pogo_det_gpio)	{
		gpio = gpio_get_value(smb1357data->pogo_det_gpio);
		dev_info(&client->dev,"health: pogo gpio %d\n",gpio);
	}

	if (charger->is_charging == true) {
#if defined(CHECK_VF_BY_IRQ)
		handle_nested_irq(smb1357data->detbat_irq);
#endif
		if (charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK) {
			if (irq_e & IRQ_E_DC_UV_BIT) {
				health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
				if (gpio == true) {
					dev_info(&client->dev,
					"health: need to check pogo event cnt %d\n",
						smb1357data->pogo_det_count);
					if ((smb1357data->pogo_det_count == 0)&&
						(smb1357data->pogo_status != DCIN_NONE)) {
						cancel_delayed_work(&charger->isr_work);
						schedule_delayed_work(&charger->isr_work,
							msecs_to_jiffies(0));
					}
				}
			}
			else if (irq_e & IRQ_E_DC_OV_BIT) {
				health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			}
		}
		else {
			if (irq_e & IRQ_E_USB_UV_BIT)
				health = POWER_SUPPLY_HEALTH_UNDERVOLTAGE;
			else if (irq_e & IRQ_E_USB_OV_BIT)
				health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		}

		if ((smb1357data->siop_level >= 100) &&
			((charger->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK))) {
#if defined(ENABLE_CHG_THERM_LIMIT)
			int temp = 0;
			temp = smb1357_get_therm_temp(client);

			if (smb1357data->chg_limit == false) {
				if (smb1357data->chg_high_temp < temp){
					smb1357data->chg_limit = true;
					if (smb1357data->sysfs_mode == USE_SYSFS_CUR){
						smb1357_set_input_current_limit(client,
							smb1357data->sysfs_curin);
						smb1357_set_charging_current(client,
							smb1357data->sysfs_curout);
					}
					else {
						smb1357_set_input_current_limit(client,
							CHG_THERM_INPUT_LIMIT_CURRENT);
						smb1357_set_charging_current(client,
							CHG_THERM_CHARGING_LIMIT_CURRENT);
					}
					dev_info(&client->dev,"%s chg set limit temp %d\n",
						__func__, temp);
				}
			}

			if (smb1357data->chg_limit == true) {
				if (smb1357data->chg_high_temp_recovery > temp) {
					smb1357data->chg_limit = false;
					smb1357_set_input_current_limit(client,
						charger->pdata->charging_current
						[charger->cable_type].input_current_limit);
					smb1357_set_charging_current(client,
						charger->pdata->charging_current
						[charger->cable_type].fast_charging_current);
					dev_info(&client->dev,"%s chg limit free temp %d\n",
						__func__, temp);
				}
				else if (CHG_THERM_REDUCE_MAX_TEMP < temp)
				{
					smb1357_set_input_current_limit(client,
						SIOP_INPUT_LIMIT_CURRENT);
					smb1357_set_charging_current(client,
						SIOP_CHARGING_LIMIT_CURRENT);
					dev_info(&client->dev,
						"%s WARNING!! OVER THE MAX TEMP\n", __func__);
				}
			}
#endif
		}
#if defined(TIMER_FORCE_BLOCK)
		if ((charger->is_charging == true) &&
			((charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK))) {
			if ((force_block_count >= FORCE_BLOCK_MAX) &&
				force_block == false) {
				force_block = true;
				force_block_count = 0;
				smb1357_set_charging_current(client,2100);
			}
			else if((force_block_count >= FORCE_BLOCK_MIN)&&
				force_block == true) {
				force_block = false;
				force_block_count = 0;
				smb1357_set_charging_current(client,3000);
			}
			force_block_count++;

			dev_dbg(&client->dev,"%s timer check %d, block %d\n",
				__func__, force_block_count, force_block);
		}
		else {
			force_block = false;
			force_block_count = 0;
		}
#endif
	}

	if (status_1 & 0x08){
		if (smb1357data->ovp_gpio_en) {
			ovp_gpio_enable(smb1357data->ovp_gpio_en, true);
			msleep(500);
			ovp_gpio_enable(smb1357data->ovp_gpio_en, false);
		}
		dev_dbg(&client->dev,"USBIN suspended %s\n",__func__);
	}

	dev_info(&client->dev,"health:S1[0x%2x]S4[0x%2x]S8[0x%2x]",
		status_1, status_4, status_8);
	dev_info(&client->dev,"health:irqE[0x%2x]en[0x%2x]cur[0x%2x]\n",
		irq_e, chg_en, chg_cur);

	return (int)health;
}


static void smb1357_enter_suspend(struct i2c_client *client)
{
	pr_info("%s: ENTER SUSPEND\n", __func__);

	smb1357_set_input_current_limit(client, 0x00);
	smb1357_set_charging_current(client, 0x00);
	smb1357_enable_charging(client, false);
	smb1357_enable_otg(client, false);

	/* When the battery removed. It will be shutdown the device.
	 * Set min input current
	 * Disable Charging.
	 */
	if (smb1357_check_vf_status(client) == false) {
		smb1357_chgin_shutdown(client, true);
		msleep(100);
	}
	else {
		smb1357_chgin_shutdown(client, false);
	}
}

static void smb1357_charger_function_control(
				struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data = i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;

	union power_supply_propval val;
	u8 usb_type, usb_mode, term, status, data_f;
	union power_supply_propval input_value;

	if (charger->charging_current < 0) {
		dev_dbg(&client->dev,
			"%s : OTG is activated. Ignore command!\n", __func__);
		return;
	}

	/* [STEP - 1] ================================================
	* Volatile write permission(bit 6) - allow(1)
	*/
	smb1357_write_enable(charger->client, true);

	psy_do_property("battery", get,
		POWER_SUPPLY_PROP_HEALTH, input_value);

	if (input_value.intval == POWER_SUPPLY_HEALTH_UNSPEC_FAILURE) {
		pr_info("[SMB1357] Unspec_failure, charger suspend\n");
		smb1357_enter_suspend(client);
	}
	else if (charger->cable_type ==	POWER_SUPPLY_TYPE_BATTERY) {
		cancel_delayed_work(&smb1357data->hvdcp_det_work);

		/* Charger Disabled */
		smb1357_enable_charging(client, false);

		pr_info("[SMB1357] Set the registers to the default configuration\n");

		smb1357_set_default_data(client);

		smb1357_charger_i2c_read(client, IRQ_F_REG, &data_f);
		if ((data_f & IRQ_F_POWER_OK_BIT) == IRQ_F_POWER_OK_BIT) {
			/* Check USBIN/DCIN HV/LV Port */
			smb1357_charger_i2c_read(client, STATUS_8_REG, &status);
			if (((status & (USBIN_HV | USBIN_LV)) == false) &&
				((status & (DCIN_HV | DCIN_LV)) != false) &&
				(smb1357data->pogo_2nd_charging == false)) {
				/* Removed USBIN , Connected DCIN, Check TOP-OFF  */
				if (smb1357data->pogo_det_gpio) {
					wake_lock(&smb1357data->chg_wake_lock);
					cancel_delayed_work(&smb1357data->pogo_det_work);
					smb1357data->pogo_det_count = 3;
					schedule_delayed_work(&smb1357data->pogo_det_work,
						msecs_to_jiffies(200));
					dev_dbg(&client->dev,"%s, send pogo detection\n",__func__);
				}
			}
			dev_dbg(&client->dev,"%s : s8 0x%x 2nd = %d, pogo gpio %d\n",
				__func__, status, smb1357data->pogo_2nd_charging,
				gpio_get_value(smb1357data->pogo_det_gpio));
		}
	}
	else {
		int full_check_type;

		psy_do_property("battery", get,	POWER_SUPPLY_PROP_CHARGE_NOW, val);

		if (val.intval == SEC_BATTERY_CHARGING_1ST)
			full_check_type = charger->pdata->full_check_type;
		else
			full_check_type = charger->pdata->full_check_type_2nd;

		smb1357_chgin_shutdown(client, false);

        /* [STEP - 2] ================================================
        * USB 5/1(9/1.5) Mode(bit 1) - USB1/USB1.5(0), USB5/USB9(1)
        * USB/HC Mode(bit 0) - USB5/1 or USB9/1.5 Mode(0)
        *                      High-Current Mode(1)
        */
		switch (charger->cable_type) {
		case POWER_SUPPLY_TYPE_USB_CDP:
		case POWER_SUPPLY_TYPE_MISC:
		case POWER_SUPPLY_TYPE_WIRELESS:
		case POWER_SUPPLY_TYPE_CARDOCK:
		case POWER_SUPPLY_TYPE_UARTOFF:
		case POWER_SUPPLY_TYPE_MHL_900:
		case POWER_SUPPLY_TYPE_MHL_1500:
		case POWER_SUPPLY_TYPE_LAN_HUB:
		case POWER_SUPPLY_TYPE_SMART_NOTG:
			/* High-current mode */
			usb_type = USB3_VAL;
			usb_mode = USB_AC_VAL;
			break;
		case POWER_SUPPLY_TYPE_UNKNOWN:
		case POWER_SUPPLY_TYPE_MAINS:
		case POWER_SUPPLY_TYPE_HV_MAINS:
		case POWER_SUPPLY_TYPE_POGODOCK:
		case POWER_SUPPLY_TYPE_UPS:
		case POWER_SUPPLY_TYPE_USB:
		case POWER_SUPPLY_TYPE_USB_DCP:
		case POWER_SUPPLY_TYPE_USB_ACA:
		case POWER_SUPPLY_TYPE_MHL_500:
		case POWER_SUPPLY_TYPE_MHL_USB:
		case POWER_SUPPLY_TYPE_SMART_OTG:
		case POWER_SUPPLY_TYPE_POWER_SHARING:
			/* USB5 */
			usb_type = USB2_VAL;
			usb_mode = USB_500_VAL;
			break;
		default:
			/* USB1 */
			usb_type = USB2_VAL;
			usb_mode = USB_100_VAL;
			break;
		}
		smb1357_usb_current_limit(client, usb_type, usb_mode);

		/* [STEP 3] Charge Current(0x00) ===============================
		 * Set pre-charge current(bit 7:5) - 250mA(011)
		 * Set fast charge current(bit 4:0)
		*/
		switch (full_check_type) {
		case SEC_BATTERY_FULLCHARGED_CHGGPIO:
		case SEC_BATTERY_FULLCHARGED_CHGINT:
		case SEC_BATTERY_FULLCHARGED_CHGPSY:
		case SEC_BATTERY_FULLCHARGED_TIME:
			smb1357_set_cur_termination(client, false);

			if (val.intval == SEC_BATTERY_CHARGING_1ST) {
				dev_info(&client->dev,
					"1st %s : termination current (%dmA)\n",
					__func__, charger->pdata->charging_current[
					charger->cable_type].full_check_current_1st);
				term = smb1357_get_term_current_limit_data(
					charger->pdata->charging_current[
					charger->cable_type].full_check_current_1st);

				smb1357data->pogo_2nd_charging = false;
			} else {
				if (charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK)
					smb1357data->pogo_2nd_charging = true;
#ifndef SECOND_TERMINATION_CURRENT
				dev_info(&client->dev,
					"2st %s : termination current (%dmA)\n",
					__func__, charger->pdata->charging_current[
					charger->cable_type].full_check_current_2nd);
				term = smb1357_get_term_current_limit_data(
					charger->pdata->charging_current[
					charger->cable_type].full_check_current_2nd);
#else
				/* Disable Charging */
				smb1357_enable_charging(client, false);

				dev_info(&client->dev,
					"2st %s : termination current (%dmA)\n",
					__func__, charger->pdata->charging_current[
					charger->cable_type].full_check_current_1st
					/ SECOND_TERMINATION_CURRENT);
				term = smb1357_get_term_current_limit_data(
					charger->pdata->charging_current[
					charger->cable_type].full_check_current_1st
					/ SECOND_TERMINATION_CURRENT);
#endif
			}

			/* termination level */
			smb1357_charger_masked_write_reg(client, CFG_3_REG,
						CHG_ITERM_MASK, term);

			/* enable termination */
			smb1357_set_cur_termination(client, true);
			break;
		}

		/* Set charging current */
		smb1357_set_charging_current(client, charger->charging_current);

		/* [STEP - 4] ===============================================
		 * Set Input Current limit : default 1600mA
		 * */

		dev_info(&client->dev, "%s : input current (%dmA)\n",
			__func__, charger->pdata->charging_current
			[charger->cable_type].input_current_limit);

		if ((charger->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK)) {
			smb1357_set_input_current_limit(client,
				(smb1357data->siop_level < 100) ?
				SIOP_CHARGING_LIMIT_CURRENT :
				charger->pdata->charging_current
				[charger->cable_type].input_current_limit);
		}
		else {
			smb1357_set_input_current_limit(client,
				charger->pdata->charging_current
				[charger->cable_type].input_current_limit);
		}

		/* [STEP - 5] =================================================
		 * Disable Pin Control, Enable CMD control
		 * USB5/1/HC input State(bit0) - Dual-state input(1)
		*/
		smb1357_charger_masked_write_reg(client, CFG_E_REG,
				USB_CTRL_BY_PIN_BIT|USB_DUAL_STATE_BIT,
				0x00|USB_DUAL_STATE_BIT);

		/* [STEP - 6] =================================================
		 * AICL - Enalbed(1)
		 */
		if (charger->pdata->chg_functions_setting &
			SEC_CHARGER_NO_GRADUAL_CHARGING_CURRENT) {
			/* disable AICL */
			smb1357_charger_masked_write_reg(client, CFG_D_REG,
				USBIN_AICL_BIT, 0x00);
		}
		else {
			/* disable AICL */
			smb1357_charger_masked_write_reg(client, CFG_D_REG,
				USBIN_AICL_BIT, 0x00);

			msleep(50);
			/* enable AICL */
			smb1357_charger_masked_write_reg(client, CFG_D_REG,
				USBIN_AICL_BIT, USBIN_AICL_BIT);
		}

		/* [STEP - 7] =================================================
		 * Float Voltage(bit 5:0)
		*/
		smb1357_set_vfloat(client, charger->pdata->chg_float_voltage);

		/* [STEP - 8] =================================================
		 * Charge control
		 * Automatic Recharge disable,
		 * Current Termination disable,
		 * APSD enable
		*/
		if (charger->cable_type == POWER_SUPPLY_TYPE_MAINS) {
		 /* Sometimes It didn't detected TA_HV.
		  * Don't move below code to other point.
		  */
			u8 hvdcp_type = 0;
			smb1357_charger_masked_write_reg(client, CFG_E_REG,
				HVDCP_ADAPTER_MASK|HVDCP_ENABLE_BIT,
				HVDCP_ADAPTER_9V|HVDCP_ENABLE_BIT);

			msleep(100);

			smb1357_charger_i2c_read(client, STATUS_7_REG, &hvdcp_type);
			if (hvdcp_type == 0x00) {
				/* USBIN ADAPTER : 9V only */
				smb1357_charger_masked_write_reg(client, CFG_C_REG,
					USBIN_ADAPTER_MASK, 0x60);
			}
		}

		if ((charger->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK)) {
			smb1357_set_APSD(client, true);
		}
		else {
			smb1357_set_APSD(client, false);
		}

		/* USBIN ADAPTER : 5V or 9V */
		smb1357_charger_masked_write_reg(client, CFG_C_REG,
			USBIN_ADAPTER_MASK, 0x20);

		smb1357_set_auto_recharge(client, true);

		/* [STEP - 9] =================================================
		 *  STAT active low(bit 1),
		*/
		smb1357_set_STAT_conf(client);

		/* [STEP - 10] =================================================
		 * Mininum System Voltage(bit 6) - 3.75v
		*/
		smb1357_set_VSYS(client);

		/* [STEP - 11] ================================================
		 * Low-Battery/SYSOK Voltage threshold - 3.25V
		 */
		smb1357_lowbatt_threshold(client);

		/* [STEP - 12] ================================================
		 * Hard/Soft Limit Cell temp monitor
		*/
		smb1357_temp_compensation(client);

		/* [STEP - 13] ================================================
		 * STATUS ingerrupt - Clear
		*/
		smb1357_clear_irq_status(client);

		/* [STEP - 14] ================================================
		 * Interrupt - Configure.
		*/
		smb1357_irq_cfg(client);

		/* Enable batt irq */
		smb1357_enable_batt_irq(client, false);

		/* [STEP - 15] ================================================
		 * Volatile write permission(bit 6) - allowed(1)
		 * Charging Enable(bit 1) - Enabled(1)
		*/
		if (charger->charging_current > 0)
			smb1357_enable_charging(client, true);
		else
			smb1357_enable_charging(client, false);

		/* Start HVDCP Detect WorkQueue */
		if (charger->cable_type == POWER_SUPPLY_TYPE_MAINS){
			smb1357data->hvdcp_det_count = 3;
			cancel_delayed_work(&smb1357data->hvdcp_det_work);
			schedule_delayed_work(&smb1357data->hvdcp_det_work,
				msecs_to_jiffies(3000));
		}
		else if (charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK) {
			/* DCIN enable AICL */
			smb1357_charger_masked_write_reg(client, CFG_B_REG,
				DCIN_AICL_BIT, DCIN_AICL_BIT);
		}
		else {
			cancel_delayed_work(&smb1357data->hvdcp_det_work);
			/* DCIN enable AICL */
			smb1357_charger_masked_write_reg(client, CFG_B_REG,
				DCIN_AICL_BIT, 0x00);
		}
	}

	dev_info(&client->dev,"%s : %d\n", __func__,__LINE__);
}

static void smb1357_charger_otg_control(
				struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data =
			i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;
	union power_supply_propval value;
	u8 enable = false;

	switch (charger->cable_type) {
		case POWER_SUPPLY_TYPE_POWER_SHARING:
			psy_do_property("ps", get,
				POWER_SUPPLY_PROP_STATUS, value);
			if (value.intval)
				enable = true;
			dev_info(&client->dev, "%s : PS %s\n",
				__func__, ((enable) ? "Enable":"Disable"));
		case POWER_SUPPLY_TYPE_OTG:
			/* Enable OTG */
#if defined(ENABLE_SHUTDOWN_MODE)
			if (smb1357data->shdn_gpio) {
				gpio_set_value(smb1357data->shdn_gpio, true);
				msleep(10);
			}
#endif
			/* OTG output current limit : 250mA */
			smb1357_charger_masked_write_reg(client, USBIN_DCIN_CFG_REG,
				OTG_CURRENT_LIMIT_MASK, 0x00);

			/* OTG Under Voltage */
			smb1357_usb_current_limit(client, USB3_VAL, USB_500_VAL);

			/* OTG Over Current IRQ */
			smb1357_enable_irq(client, IRQ_CFG_REG,
				IRQ_OTG_OV_CURRENT_BIT,	IRQ_OTG_OV_CURRENT_BIT);

			/* OTG output current limit : 1000mA */
#if defined(CONFIG_SEC_FACTORY)
			smb1357_charger_masked_write_reg(client, USBIN_DCIN_CFG_REG,
				OTG_CURRENT_LIMIT_MASK,	0x04);
#else
			smb1357_charger_masked_write_reg(client, USBIN_DCIN_CFG_REG,
				OTG_CURRENT_LIMIT_MASK,	OTG_CURRENT_LIMIT_MASK);
#endif
			if (charger->cable_type == POWER_SUPPLY_TYPE_OTG) {
				enable = true;
			}

			/* Enable otg */
			if (smb1357data->revision < CS21_REVISION) {
				smb1357_enable_otg(client, enable);
				smb1357_enable_otg(client, false);
			}
			smb1357_enable_otg(client, enable);
			break;
		default:
			/* Disable OTG */
			/* OTG Over Current IRQ */
			smb1357_enable_irq(client, IRQ_CFG_REG,
				IRQ_OTG_OV_CURRENT_BIT,	0x00);

			/* disable otg */
			if (smb1357data->revision < CS21_REVISION) {
				smb1357_enable_otg(client, false);
			}
			smb1357_enable_otg(client, false);

			if (smb1357data->revision < CS21_REVISION) {
				if (smb1357data->ovp_gpio_en) {
					ovp_gpio_enable(smb1357data->ovp_gpio_en, true);
					msleep(30);
					ovp_gpio_enable(smb1357data->ovp_gpio_en, false);
				}
			}
			break;
	}

	dev_info(&client->dev, "%s : OTG [%s] \n",__func__,
		((enable) ?	"ON":"OFF"));
	return;
}

void smb1357_charger_shutdown(struct i2c_client *client)
{
	pr_info("%s: smb1357 Charging Disabled\n", __func__);
	
	smb1357_charger_masked_write_reg(client, CFG_E_REG,
		HVDCP_ADAPTER_MASK, HVDCP_ADAPTER_5V);
	return;
}

static int smb1357_debugfs_show(struct seq_file *s, void *data)
{
	struct sec_charger_info *charger = s->private;
	u8 reg;
	u8 reg_data;

	seq_printf(s, "SMB CHARGER IC :\n");
	seq_printf(s, "==================\n");
	for (reg = 0x00; reg <= 0x1F; reg++) {
		smb1357_charger_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	for (reg = 0x40; reg <= 0x57; reg++) {
		smb1357_charger_i2c_read(charger->client, reg, &reg_data);
		seq_printf(s, "0x%02x:\t0x%02x\n", reg, reg_data);
	}

	seq_printf(s, "\n");
	return 0;
}

static int smb1357_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, smb1357_debugfs_show, inode->i_private);
}

static const struct file_operations smb1357_debugfs_fops = {
	.open           = smb1357_debugfs_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
};

bool smb1357_hal_chg_init(struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data = i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;

	dev_info(&client->dev,
		"%s: SMB1357 Charger init(Start)!!\n", __func__);

	/* sys-kernel-debug-smb1357_regs */
	(void) debugfs_create_file("smb1357_regs",
		S_IRUGO, NULL, (void *)charger, &smb1357_debugfs_fops);

	return true;
}

bool smb1357_hal_chg_suspend(struct i2c_client *client)
{
#if defined(ENABLE_SHUTDOWN_MODE)
	struct smb1357_charger_data *smb1357data = i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;

	if (charger->is_charging == false) {
		if (smb1357data->shdn_gpio) {
			gpio_set_value(smb1357data->shdn_gpio, false);
		}
	}
#endif
	return true;
}

bool smb1357_hal_chg_resume(struct i2c_client *client)
{
#if defined(ENABLE_SHUTDOWN_MODE)
	struct smb1357_charger_data *smb1357data = i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;

	if (charger->is_charging == false) {
		if (smb1357data->shdn_gpio) {
			gpio_set_value(smb1357data->shdn_gpio, true);
		}
	}
#endif
	return true;
}

bool smb1357_hal_chg_get_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      union power_supply_propval *val)
{
	struct smb1357_charger_data *smb1357data = i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;
	u8 port;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = charger->cable_type;
		if ((charger->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS)) {
			port = smb1357_get_charger_type(charger->client);
			switch(port){
				case HVDCP_A:
				case HVDCP_12V:
				case HVDCP_9V:
				case HVDCP_5V:
					val->intval = POWER_SUPPLY_TYPE_HV_MAINS;
					break;
				default:
					break;
			}
		}
		else if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY) {
			u8 data_f;
			smb1357_charger_i2c_read(client, IRQ_F_REG, &data_f);
			if ((data_f & IRQ_F_POWER_OK_BIT) == IRQ_F_POWER_OK_BIT) {
				val->intval = POWER_SUPPLY_TYPE_MAINS;
			}
			dev_info(&client->dev,
			"%s:data_f 0x%x, val = %d!\n", __func__, data_f, val->intval);
		}
		break;

	case POWER_SUPPLY_PROP_STATUS:
		val->intval = smb1357_get_charging_status(client);
		break;

	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		if (charger->is_charging)
			val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
		else
			val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = smb1357_get_charging_health(client);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		/* check removed battery */
		val->intval = smb1357_check_vf_status(client);
		break;
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
		if (charger->charging_current) {
			val->intval = smb1357_status_charging_current(client);
		} else
			val->intval = 0;

		dev_dbg(&client->dev,
			"%s : set-current(%dmA), current now(%dmA)\n",
			__func__, charger->charging_current, val->intval);
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		smb1357_charger_i2c_read(client, VFLOAT_REG, &port);
		val->intval = port;
		pr_info("%s: Float voltage : 0x%x\n", __func__, val->intval);
		break;
#endif
	default:
		return false;
	}
	return true;
}

bool smb1357_hal_chg_set_property(struct i2c_client *client,
			      enum power_supply_property psp,
			      const union power_supply_propval *val)
{
	struct smb1357_charger_data *smb1357data = i2c_get_clientdata(client);
	struct sec_charger_info *charger = smb1357data->charger;
	bool ret = true;

	mutex_lock(&smb1357data->mutex);

	switch (psp) {
	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		dev_dbg(&client->dev,
				"%s : cable type %d!\n", __func__, val->intval);

		if (val->intval != POWER_SUPPLY_TYPE_POGODOCK) {
			smb1357data->usbin_cable_type = val->intval;
			dev_dbg(&client->dev,
				"%s : usbin cable type %d!\n", __func__, val->intval);
		}

		if (charger->charging_current < 0) {
			smb1357_charger_otg_control(client);
		}
		else if (charger->charging_current > 0) {
			smb1357_charger_function_control(client);
		}
		else {
			smb1357_charger_function_control(client);
			smb1357_charger_otg_control(client);
		}
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		smb1357_set_input_current_limit(client, val->intval);
		break;
	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		smb1357_set_charging_current(client, val->intval);
		break;
	default:
		ret = false;
		break;
	}

	mutex_unlock(&smb1357data->mutex);
	return ret;
}

ssize_t smb1357_hal_chg_show_attrs(struct device *dev,
				const ptrdiff_t offset, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int i = 0;
	char *str = NULL;

	switch (offset) {
	case CHG_REG:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_addr);
		break;
	case CHG_DATA:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%x\n",
			chg->reg_data);
		break;
	case CHG_REGS:
		str = kzalloc(sizeof(char)*1024, GFP_KERNEL);
		if (!str)
			return -ENOMEM;

		smb1357_read_regs(chg->client, str);
		i += scnprintf(buf + i, PAGE_SIZE - i, "%s\n",
			str);

		kfree(str);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t smb1357_hal_chg_store_attrs(struct device *dev,
				const ptrdiff_t offset,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *chg =
		container_of(psy, struct sec_charger_info, psy_chg);
	int ret = 0;
	u8 data = 0;
	int x = 0;

	switch (offset) {
	case CHG_REG:
		if (sscanf(buf, "%x\n", &x) == 1) {
			chg->reg_addr = x;
			smb1357_charger_i2c_read(chg->client,
				chg->reg_addr, &data);
			chg->reg_data = data;
			dev_dbg(dev, "%s: (read) addr = 0x%x, data = 0x%x\n",
				__func__, chg->reg_addr, chg->reg_data);
			ret = count;
		}
		break;
	case CHG_DATA:
		if (sscanf(buf, "%x\n", &x) == 1) {
			data = (u8)x;
			dev_dbg(dev, "%s: (write) addr = 0x%x, data = 0x%x\n",
				__func__, chg->reg_addr, data);
			smb1357_charger_i2c_write(chg->client,
				chg->reg_addr, &data);
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static struct device_attribute smb1357_charger_attrs[] = {
	SMB1357_CHARGER_ATTR(reg),
	SMB1357_CHARGER_ATTR(data),
	SMB1357_CHARGER_ATTR(regs),
	POGO_ATTR(pogo),
	CHIP_ID_CHECK_ATTR(chip_id),
	CHG_THERM_ATTR(chg_therm),
	CHG_THERM_ADC_ATTR(chg_therm_adc),
	CHG_CURRENT_ATTR(cur_mode),
	CHG_CURRENT_ATTR(cur_set),
};

static enum power_supply_property smb1357_charger_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_AVG,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
#if defined(CONFIG_BATTERY_SWELLING)
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
#endif
};

static int smb1357_chg_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:	/* input current limit set */
		val->intval = charger->charging_current_max;
		break;
#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
#endif
	case POWER_SUPPLY_PROP_PRESENT:
	case POWER_SUPPLY_PROP_ONLINE:
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
	case POWER_SUPPLY_PROP_HEALTH:
	case POWER_SUPPLY_PROP_CURRENT_AVG:	/* charging current */
	/* calculated input current limit value */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!smb1357_hal_chg_get_property(charger->client, psp, val))
			return -EINVAL;
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int smb1357_chg_set_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    const union power_supply_propval *val)
{
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&charger->client->dev);
	union power_supply_propval input_value;
	union power_supply_propval charging_value;
	int set_charging_current, set_charging_current_max;
	const int usb_charging_current = charger->pdata->charging_current[
		POWER_SUPPLY_TYPE_USB].fast_charging_current;
#if defined(CONFIG_BATTERY_SWELLING)
	u8 reg_data;
#endif

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		charger->status = val->intval;
		break;

	/* val->intval : type */
	case POWER_SUPPLY_PROP_ONLINE:
		/* change current cable type */
		charger->cable_type = val->intval;

		set_charging_current =
			charger->pdata->charging_current[
			charger->cable_type].fast_charging_current;
		set_charging_current_max =
			charger->pdata->charging_current[
			charger->cable_type].input_current_limit;

		if (charger->cable_type == POWER_SUPPLY_TYPE_BATTERY){
			charger->is_charging = false;
		}
		else {
			charger->is_charging = true;

			/* current setting */
			if ((charger->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
				(charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
				(charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK)) {
				/* charging current after compensation of siop */
				if (smb1357data->siop_level == 0) {
					set_charging_current = usb_charging_current;
				}
				else {
					set_charging_current =
					(set_charging_current * smb1357data->siop_level) / 100;
				}

				if ((set_charging_current > 0) &&
					(set_charging_current < usb_charging_current)){
					set_charging_current = usb_charging_current;
				}

				/* input current after compenation of siop */
				if (smb1357data->siop_level < 100) {
					if (set_charging_current_max >
						SIOP_INPUT_LIMIT_CURRENT) {
						set_charging_current_max =
							SIOP_INPUT_LIMIT_CURRENT;
					}
					if (set_charging_current >
						SIOP_CHARGING_LIMIT_CURRENT) {
						set_charging_current =
							SIOP_CHARGING_LIMIT_CURRENT;
					}
				}
			}
		}

		/* charging current after compensation of siop */
		charger->charging_current = set_charging_current;

		/* input current after compenation of siop */
		charger->charging_current_max = set_charging_current_max;

		dev_info(&charger->client->dev,"%s siop= %d,chg_cur %d, in_cur %d\n",
			__func__, smb1357data->siop_level, charger->charging_current,
			set_charging_current);

		if (!smb1357_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : input current limit set */
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		charger->charging_current_max = val->intval;
	/* to control charging current,
	 * use input current limit and set charging current as much as possible
	 * so we only control input current limit to control charge current
	 */
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!smb1357_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : charging current */
	case POWER_SUPPLY_PROP_CURRENT_AVG:
		charger->charging_current = val->intval;

		if (!smb1357_hal_chg_set_property(charger->client, psp, val))
			return -EINVAL;
		break;

	/* val->intval : SIOP level (%)
	 * SIOP charging current setting
	 */
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
		/* change val as charging current by SIOP level
		 * do NOT change initial charging current setting
		 */
		smb1357data->siop_level = val->intval;

		if (charger->is_charging == true) {
		input_value.intval =
			charger->pdata->charging_current[
			charger->cable_type].input_current_limit;

		charging_value.intval =
			charger->pdata->charging_current[
			charger->cable_type].fast_charging_current;

		if ((charger->cable_type == POWER_SUPPLY_TYPE_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_HV_MAINS) ||
			(charger->cable_type == POWER_SUPPLY_TYPE_POGODOCK)) {
#if defined(REDUCE_TMM_CHG)
			if (smb1357data->siop_level == TMM_SIOP_LVL) {
				/* enforce only 5V charging */
				smb1357_charger_masked_write_reg(charger->client, CFG_E_REG,
					HVDCP_ADAPTER_MASK, HVDCP_ADAPTER_5V);

				/* no change of input limit current
				 * set charging current */
				charging_value.intval = TMM_CHG_CURRENT;
			}
			else {
				/* enable 9V charging */
				smb1357_charger_masked_write_reg(charger->client, CFG_E_REG,
					HVDCP_ADAPTER_MASK, HVDCP_ADAPTER_9V);

				if (smb1357data->siop_level < 100) {
					input_value.intval = SIOP_INPUT_LIMIT_CURRENT;
				}

				if (smb1357data->siop_level == 0) {
					charging_value.intval = usb_charging_current;
				}
				else {
					charging_value.intval =
					(charging_value.intval * smb1357data->siop_level) / 100;
				}

			/* charging current should be over than USB charging current */
				if ((charging_value.intval > 0) &&
						(charging_value.intval < usb_charging_current)){
					charging_value.intval = usb_charging_current;
				}

				if ((smb1357data->siop_level < 100) &&
					(charging_value.intval > SIOP_CHARGING_LIMIT_CURRENT)){
					charging_value.intval = SIOP_CHARGING_LIMIT_CURRENT;
				}
			}
#else
			if (smb1357data->siop_level < 100){
				input_value.intval = SIOP_INPUT_LIMIT_CURRENT;
			}

			if (smb1357data->siop_level == 0) {
				charging_value.intval = usb_charging_current;
			}
			else {
				charging_value.intval =
				(charging_value.intval * smb1357data->siop_level) / 100;
			}

			/* charging current should be over than USB charging current */
			if ((charging_value.intval > 0) &&
					(charging_value.intval < usb_charging_current)){
				charging_value.intval = usb_charging_current;
			}

			if ((smb1357data->siop_level < 100) &&
				(charging_value.intval > SIOP_CHARGING_LIMIT_CURRENT)){
				charging_value.intval = SIOP_CHARGING_LIMIT_CURRENT;
			}
#endif
		}

		/* input current after compenation of siop */
		charger->charging_current_max = input_value.intval;

		/* charging current after compensation of siop */
		charger->charging_current = charging_value.intval;

		dev_info(&charger->client->dev,"%s siop= %d,chg_cur %d, in_cur %d\n",
			__func__, smb1357data->siop_level, charger->charging_current,
			charger->charging_current_max);

			/* set input current limit */
			if (!smb1357_hal_chg_set_property(charger->client,
				POWER_SUPPLY_PROP_CURRENT_NOW, &input_value))
				return -EINVAL;
			/* set charging current as new value */
			if (!smb1357_hal_chg_set_property(charger->client,
				POWER_SUPPLY_PROP_CURRENT_AVG, &charging_value))
				return -EINVAL;
		}
		break;

#if defined(CONFIG_BATTERY_SWELLING)
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		pr_info("%s: float voltage(%d)\n", __func__, val->intval);
		smb1357_set_vfloat(charger->client, val->intval);
		smb1357_charger_i2c_read(charger->client, VFLOAT_REG, &reg_data);
		pr_info("%s: Float voltage set to : 0x%x\n", __func__, reg_data);
		break;
#endif
	default:
		return -EINVAL;
	}
	return 0;
}

static bool smb1357_is_pogo_event(struct i2c_client *client)
{
	struct smb1357_charger_data *smb1357data =
						i2c_get_clientdata(client);
	u8 data_f, data_e, status;
	bool ret = false;
	u8 gpio = false;

	msleep(1);
	/* Check Inserted Pogo  */
	smb1357_charger_i2c_read(client, IRQ_F_REG, &data_f);
	/* Check Removed Pogo */
	smb1357_charger_i2c_read(client, IRQ_E_REG, &data_e);
	/* Check DCIN HV/LV Port */
	smb1357_charger_i2c_read(client, STATUS_8_REG, &status);

	if (((data_f & IRQ_F_POWER_OK_BIT) && ((status & (DCIN_HV|DCIN_LV)) != 0)
		&& (smb1357data->pogo_status == DCIN_NONE)) ||
		((data_e & IRQ_E_DC_UV_BIT) && ((status & (DCIN_HV|DCIN_LV)) == 0)
		&&(smb1357data->pogo_status != DCIN_NONE))) {
			ret = true;
	}

	if (smb1357data->pogo_det_gpio)	{
		gpio = gpio_get_value(smb1357data->pogo_det_gpio);
		if ((smb1357data->pogo_status != DCIN_NONE) && (gpio == true))
		{
			ret = true;
			dev_info(&client->dev,"%s, pogo status[%d],gpio[%d]",
				__func__, smb1357data->pogo_status, gpio);
		}
	}

	dev_info(&client->dev,
		"%s, f[0x%x] e[0x%x] st[0x%x] gpio[%d]ret[%s]\n",
		__func__, data_f, data_e, status,smb1357data->pogo_status,
		ret ? "POGO":"NONE");

	return ret;
}

static void smb1357_chg_isr_work(struct work_struct *work)
{
	struct sec_charger_info *charger =
		container_of(work, struct sec_charger_info, isr_work.work);
	struct i2c_client *client = charger->client;
	struct smb1357_charger_data *smb1357data =
							i2c_get_clientdata(client);
	u8 data_f = 0;

	dev_info(&charger->client->dev,
		"%s: Charger Interrupt\n", __func__);
	wake_lock(&smb1357data->chgisr_wake_lock);

	if (smb1357_is_pogo_event(client)) {
		if (smb1357data->pogo_det_gpio)	{
			wake_lock(&smb1357data->chg_wake_lock);
			smb1357_syson_sensing(client, 10, true);
			cancel_delayed_work(&smb1357data->pogo_det_work);
			smb1357data->pogo_det_count = 3;
			schedule_delayed_work(&smb1357data->pogo_det_work,
				msecs_to_jiffies(200));
		}
	}

	if (smb1357data->pogo_status == DCIN_5V) {
		smb1357_dcin_config_hv_lv(client,false);
	}

	smb1357_charger_i2c_read(client, IRQ_F_REG, &data_f);
	if (data_f & IRQ_F_OTG_OV_CUR_BIT) {
#ifdef SMB_OTG_NOTIFY_OVERCURRENT
		sec_otg_notify(HNOTIFY_OVERCURRENT);
#endif
		dev_info(&client->dev,"%s, otg over current\n",__func__);
	}

	/* Clear Irq */
	smb1357_clear_irq_status(client);
	wake_unlock(&smb1357data->chgisr_wake_lock);
	return;
}

static irqreturn_t smb1357_chg_irq_thread(int irq, void *irq_data)
{
	struct sec_charger_info *charger = irq_data;

	cancel_delayed_work(&charger->isr_work);
	schedule_delayed_work(&charger->isr_work, msecs_to_jiffies(0));

	return IRQ_HANDLED;
}

#if defined(ENABLE_MAX77804K_CHG_IRQ)
static void smb1357_detbat_work(struct work_struct *work)
{
	struct smb1357_charger_data *smb1357data =
		container_of(work, struct smb1357_charger_data, detbat_work.work);
	struct sec_charger_info *charger = smb1357data->charger;
	struct i2c_client *client = charger->client;
	union power_supply_propval value;

#if defined(ENABLE_SMBCHG_BATT_DET)
	value.intval = smb1357_get_batt_status(client);
#else
	value.intval = smb1357_check_vf_status(client);
#endif
	psy_do_property("battery", set,
			POWER_SUPPLY_PROP_PRESENT, value);

	dev_dbg(&client->dev, "%s:\n", __func__);
	return;
}

static irqreturn_t smb1357_detbat_irq_thread(int irq, void *irq_data)
{
	struct sec_charger_info *charger = irq_data;
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&charger->client->dev);

	schedule_delayed_work(&smb1357data->detbat_work, msecs_to_jiffies(0));

	return IRQ_HANDLED;
}
#endif

static int smb1357_chg_create_attrs(struct device *dev)
{
	int i, rc;

	for (i = 0; i < ARRAY_SIZE(smb1357_charger_attrs); i++) {
		rc = device_create_file(dev, &smb1357_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	goto create_attrs_succeed;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &smb1357_charger_attrs[i]);
create_attrs_succeed:
	return rc;
}

ssize_t smb1357_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - smb1357_charger_attrs;
	int i = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
	case CHG_REGS:
		i = smb1357_hal_chg_show_attrs(dev, offset, buf);
		break;
	default:
		i = -EINVAL;
		break;
	}

	return i;
}

ssize_t smb1357_chg_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - smb1357_charger_attrs;
	int ret = 0;

	switch (offset) {
	case CHG_REG:
	case CHG_DATA:
		ret = smb1357_hal_chg_store_attrs(dev, offset, buf, count);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

ssize_t chg_current_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&charger->client->dev);
	const ptrdiff_t offset = attr - smb1357_charger_attrs;
	int ret = 0;
	int mode;
	int cur = 0;
	switch (offset) {
	case CUR_MODE:
		if (sscanf(buf, "%d\n", &mode) == 1) {
			smb1357data->sysfs_mode = (u8)mode;
			ret = count;
		}
		break;
	case CUR_SET:
		if (sscanf(buf, "%d\n", &cur) == 1) {
			switch(smb1357data->sysfs_mode) {
			case 1:
				smb1357data->sysfs_curin = (u32)cur;
				break;
			case 2:
				smb1357data->sysfs_curout = (u32)cur;
				break;
#if defined(USE_SYSFS_CURRENT_CTRL)
			case 11:
				charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_MAINS].input_current_limit = (u32)cur;
				break;
			case 12:
				charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_MAINS].fast_charging_current = (u32)cur;
				break;
			case 21:
				charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_HV_MAINS].input_current_limit = (u32)cur;
				break;
			case 22:
				charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_HV_MAINS].fast_charging_current = (u32)cur;
				break;
#endif
			default:
					break;
			}
			ret = count;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}

	dev_info(&charger->client->dev,
		"%s: mode [%d], cur[%d]\n", __func__,
		smb1357data->sysfs_mode,(u32)cur);

	return ret;
}

ssize_t chg_current_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&charger->client->dev);
	const ptrdiff_t offset = attr - smb1357_charger_attrs;
	int ret = 0;
	u32 curin = 0;
	u32 curout = 0;
	switch (offset) {
	case CUR_MODE:
		ret=sprintf(buf,"%d\n",smb1357data->sysfs_mode);
		break;
	case CUR_SET:
		switch(smb1357data->sysfs_mode) {
		case 1:
		case 2:
			curin = smb1357data->sysfs_curin;
			curout = smb1357data->sysfs_curout;
			ret=sprintf(buf,"mode[%d] in: %d, out: %d\n",
			smb1357data->sysfs_mode, curin, curout);
			break;
		case 11:
		case 12:
			curin =	charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_MAINS].input_current_limit;
			curout = charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_MAINS].fast_charging_current;
			ret=sprintf(buf,"mode[%d] in: %d, out: %d\n",
			smb1357data->sysfs_mode,curin, curout);
			break;
		case 21:
		case 22:
			curin =	charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_HV_MAINS].input_current_limit;
			curout = charger->pdata->charging_current[
				POWER_SUPPLY_TYPE_HV_MAINS].fast_charging_current;
			ret=sprintf(buf,"mode[%d] in: %d, out: %d\n",
			smb1357data->sysfs_mode,curin, curout);
			break;
		case USE_SYSFS_CUR:
			ret=sprintf(buf,"Use SYSFS CUR!!!\n");
			break;
		default:
			ret=sprintf(buf,"%s\n","cur_mode Error");
			break;
		}
		break;
	default:
		ret = -EINVAL;
		break;
	}
	dev_info(&charger->client->dev,
		"%s: mode [%d], cur in[%d]cur out[%d]\n", __func__,
		smb1357data->sysfs_mode, curin, curout);
	return ret;
}

#ifdef CONFIG_OF
static int smb1357_charger_read_u32_index_dt(const struct device_node *np,
				       const char *propname,
				       u32 index, u32 *out_value)
{
	struct property *prop = of_find_property(np, propname, NULL);
	u32 len = (index + 1) * sizeof(*out_value);

	if (!prop)
		return (-EINVAL);
	if (!prop->value)
		return (-ENODATA);
	if (len > prop->length)
		return (-EOVERFLOW);

	*out_value = be32_to_cpup(((__be32 *)prop->value) + index);

	return 0;
}

static int smb1357_charger_parse_dt(struct smb1357_charger_data *smb1357data)
{
	struct device_node *np = of_find_node_by_name(NULL, DT_NODE_NAME);
	struct sec_charger_info *charger = smb1357data->charger;
	sec_battery_platform_data_t *pdata = charger->pdata;
	int ret = 0;
	int i, len;
	const u32 *p;

	if (np == NULL) {
		pr_err("%s np NULL\n", __func__);
		return -1;
	} else {
		ret = of_get_named_gpio(np, "battery,pogo_det_gpio",0);
		if (ret < 0)
			pr_err("%s: pogo det gpio (%d)\n", __func__,ret);
		else
			smb1357data->pogo_det_gpio = ret;

		ret = of_get_named_gpio(np, "battery,shdn_gpio",0);
		if (ret < 0)
			pr_err("%s: shutdown gpio (%d)\n", __func__,ret);
		else
			smb1357data->shdn_gpio = ret;

		ret = of_get_named_gpio(np, "battery,ovp_gpio_en",0);
		if (ret < 0)
			pr_err("%s: ovp gpio en (%d)\n", __func__,ret);
		else
			smb1357data->ovp_gpio_en = ret;

		ret = of_get_named_gpio(np, "battery,chg_vf_det",0);
		if (ret < 0)
			pr_err("%s: vf det (%d)\n", __func__,ret);
		else
			smb1357data->chg_vf_det = ret;

		ret = of_property_read_u32(np, "battery,chg_high_temp",
				&smb1357data->chg_high_temp);
		if (ret < 0)
			pr_err("%s: chg_high_temp read failed (%d)\n", __func__,ret);

		ret = of_property_read_u32(np, "battery,chg_high_temp_recovery",
				&smb1357data->chg_high_temp_recovery);
		if (ret < 0)
			pr_err("%s: chg_high_temp_recovery read failed (%d)\n",
					__func__,ret);

		smb1357data->chg_vf_1p8v =
			regulator_get(&charger->client->dev, "max77826_ldo6");
		if (IS_ERR(smb1357data->chg_vf_1p8v)){
			pr_err("%s: chg_vf_1p8v regulator error\n",__func__);
			smb1357data->chg_vf_1p8v = NULL;
		}

		ret = of_property_read_u32(np, "battery,detbat_irq",
				&smb1357data->detbat_irq);
		if (ret < 0)
			pr_err("%s: chg_irq read failed (%d)\n", __func__,ret);

		ret = of_get_named_gpio(np, "battery,chg_irq", 0);
		if (ret < 0)
			pr_err("%s: chg_irq read failed (%d)\n", __func__,ret);
		else{
			gpio_request(ret, "smbcharger_irq");
			pdata->chg_irq = gpio_to_irq(ret);
		}

		ret = of_property_read_u32(np, "battery,chg_irq_attr",
				(unsigned int *)&pdata->chg_irq_attr);
		if (ret < 0)
			pr_err("%s: chg_irq_attr read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,chg_float_voltage",
					&pdata->chg_float_voltage);
		if (ret < 0)
			pr_err("%s: chg_float_voltage read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,ovp_uvlo_check_type",
					&pdata->ovp_uvlo_check_type);
		if (ret < 0)
			pr_err("%s: ovp_uvlo_check_type read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,full_check_type",
					&pdata->full_check_type);
		if (ret < 0)
			pr_err("%s: full_check_type read failed (%d)\n", __func__, ret);

		ret = of_property_read_u32(np, "battery,full_check_type_2nd",
					&pdata->full_check_type_2nd);
		if (ret < 0)
			pr_err("%s: full_check_type_2nd read failed (%d)\n", __func__, ret);

		p = of_get_property(np, "battery,input_current_limit", &len);
		len = len / sizeof(u32);
		pdata->charging_current = kzalloc(sizeof(sec_charging_current_t) * len,
						  GFP_KERNEL);

		for(i = 0; i < len; i++) {
			ret = smb1357_charger_read_u32_index_dt(np,
					 "battery,input_current_limit", i,
					 &pdata->charging_current[i].input_current_limit);
			ret = smb1357_charger_read_u32_index_dt(np,
					 "battery,fast_charging_current", i,
					 &pdata->charging_current[i].fast_charging_current);
			ret = smb1357_charger_read_u32_index_dt(np,
					 "battery,full_check_current_1st", i,
					 &pdata->charging_current[i].full_check_current_1st);
			ret = smb1357_charger_read_u32_index_dt(np,
					 "battery,full_check_current_2nd", i,
					 &pdata->charging_current[i].full_check_current_2nd);
		}
	}
	return ret;
}
#else
static int smb1357_charger_parse_dt(struct max77803_charger_data *charger)
{
	return 0;
}
#endif

static void smb1357_charger_hvdcp_det_work(struct work_struct *work)
{
	struct smb1357_charger_data *smb1357data =
	    container_of(work, struct smb1357_charger_data,hvdcp_det_work.work);
	struct sec_charger_info *charger = smb1357data->charger;
	struct i2c_client *client = charger->client;
	union power_supply_propval value;

	if (charger->is_charging == true) {
		smb1357_hal_chg_get_property(client,
				POWER_SUPPLY_PROP_ONLINE, &value);

		if ((value.intval == POWER_SUPPLY_TYPE_HV_MAINS) &&
			(charger->cable_type == POWER_SUPPLY_TYPE_MAINS)){
				psy_do_property("battery",set,
					POWER_SUPPLY_PROP_ONLINE, value);
				msleep(50);
				smb1357data->hvdcp_det_count = 0;
		}
		else {
			if (smb1357data->hvdcp_det_count > 0){
				smb1357data->hvdcp_det_count--;
				cancel_delayed_work(&smb1357data->hvdcp_det_work);
				schedule_delayed_work(&smb1357data->hvdcp_det_work,
					msecs_to_jiffies(1000));
			}
		}
	}
	dev_dbg(&client->dev,"%s, is_charging [%d], cable type = [%d]\n",
		__func__, charger->is_charging, charger->cable_type);

	return;
}

ssize_t chip_id_check_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	u8 id;

	id = smb1357_check_product_id(charger->client);

	return snprintf(buf, 9, "%s\n",
		(id == 0xff) ? "Unknown": id ? "SMB1356":"SMB1357");
}

ssize_t pogo_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&charger->client->dev);

	dev_info(&charger->client->dev,
		"%s: pogo status [ %d ]\n",
		__func__, smb1357data->pogo_status);

	return snprintf(buf, 3, "%d\n",	smb1357data->pogo_status);
}

ssize_t chg_therm_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	int temp;

	temp = smb1357_get_therm_temp(charger->client);

	dev_info(&charger->client->dev,
		"%s: chg_threm [ %d ]\n", __func__, temp);

	return snprintf(buf, 6, "%4d\n",temp);
}

ssize_t chg_therm_store_attrs(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	struct smb1357_charger_data *smb1357data =
			dev_get_drvdata(&charger->client->dev);
	int ret = 0;
	int temp;

	if (sscanf(buf, "%d\n", &temp) == 1) {
		smb1357data->chg_high_temp = temp;
		smb1357data->chg_high_temp_recovery = temp - 100;
		ret = count;
	}

	dev_info(&charger->client->dev,
		"%s: high [%d], recovery [%d]\n", __func__,
		smb1357data->chg_high_temp,smb1357data->chg_high_temp_recovery);

	return ret;
}

ssize_t chg_therm_adc_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct sec_charger_info *charger =
		container_of(psy, struct sec_charger_info, psy_chg);
	struct qpnp_vadc_result results;
	int temp_adc = -1;
	int err = 0;

	err = qpnp_vadc_read(NULL, CHG_THEM_THEM_CHANNEL, &results);

	if (err < 0) {
		dev_err(&charger->client->dev,"%s : therm read fail rc = %d\n",
			__func__, err);
	}
	else {
		temp_adc = results.adc_code;
	}

	dev_info(&charger->client->dev,
		"%s: chg_threm_adc [ %d ]\n", __func__, temp_adc);

	return snprintf(buf, 8, "%d\n",temp_adc);
}

static int sec_pogo_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct smb1357_charger_data *smb1357data =
	    container_of(psy, struct smb1357_charger_data, psy_pogo);

	if (psp != POWER_SUPPLY_PROP_ONLINE)
		return -EINVAL;

	if (smb1357data->pogo_status != DCIN_NONE)
		val->intval = 1;
	else
		val->intval = 0;

	dev_info(&smb1357data->charger->client->dev,
		"%s: pogo status [ %d ]\n",
		__func__, smb1357data->pogo_status);

	return 0;
}

static void smb1357_gpio_init(struct smb1357_charger_data *smb1357data)
{
	int ret;

	if(smb1357data->pogo_det_gpio) {
		gpio_tlmm_config(GPIO_CFG(smb1357data->pogo_det_gpio,
			0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

		gpio_request(smb1357data->pogo_det_gpio, "pogo_det_gpio");
	}

	if(smb1357data->shdn_gpio) {
		gpio_tlmm_config(GPIO_CFG(smb1357data->shdn_gpio,
			1, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

		gpio_request(smb1357data->shdn_gpio, "shdn_gpio");
#if defined(ENABLE_SHUTDOWN_MODE)
		gpio_set_value(smb1357data->shdn_gpio, true);
#else
		gpio_set_value(smb1357data->shdn_gpio, false);
#endif
		msleep(10);
	}

	if (smb1357data->ovp_gpio_en) {
		gpio_tlmm_config(GPIO_CFG(smb1357data->ovp_gpio_en,
			0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

		ret = gpio_request(smb1357data->ovp_gpio_en, "ovp_gpio_en");
		if (ret) {
			pr_err("ERROR ovp_gpio_en request failed.\n");
		}
		else {
			ovp_gpio_enable(smb1357data->ovp_gpio_en, false);
		}
	}

	if (smb1357data->chg_vf_1p8v) {
		ret = regulator_set_voltage(smb1357data->chg_vf_1p8v,
				1800000, 1800000);
		if (unlikely(ret < 0)){
			pr_err("ERROR chg_vf_1p8v set voltage failed.\n");
		}
		else {
			if (regulator_is_enabled(smb1357data->chg_vf_1p8v)){
				ret = regulator_disable(smb1357data->chg_vf_1p8v);
			}
		}
	}
	return;
}

static void smb1357_charger_pogo_det_work(struct work_struct *work)
{
	struct smb1357_charger_data *smb1357data =
	    container_of(work, struct smb1357_charger_data,pogo_det_work.work);
	struct sec_charger_info *charger = smb1357data->charger;
	struct i2c_client *client = charger->client;
	u8 gpio;
	union power_supply_propval val;

	cancel_delayed_work(&smb1357data->pogo_det_work);
	gpio = gpio_get_value(smb1357data->pogo_det_gpio);

	if (gpio == false) {
		smb1357data->pogo_det_count = 0;
		smb1357_syson_sensing(client, 4000, false);
		smb1357_dcin_config_hv_lv(client, true);
		smb1357data->pogo_status =
			smb1357_dcin_input_voltage(client);
		dev_dbg(&client->dev," %s, pogo detect, %dv usbin %d\n",
			__func__,smb1357data->pogo_status,
			smb1357data->usbin_cable_type);
		if (smb1357data->usbin_cable_type == POWER_SUPPLY_TYPE_BATTERY) {
			val.intval = POWER_SUPPLY_TYPE_POGODOCK;
			psy_do_property("battery",set,
				POWER_SUPPLY_PROP_ONLINE, val);
			msleep(100);
		}
		else {
			/* Connected USBIN port, Just update UI */
			if (smb1357data->psy_bat) {
				power_supply_changed(smb1357data->psy_bat);
				dev_dbg(&client->dev,"%s, Just update UI\n",
					__func__);
			}
		}
		wake_unlock(&smb1357data->chg_wake_lock);
	}
	else {
		if (smb1357data->pogo_det_count > 0) {
			smb1357data->pogo_det_count--;
			schedule_delayed_work(&smb1357data->pogo_det_work,
				msecs_to_jiffies(50));
		}
		else {
			dev_dbg(&client->dev," %s, pogo removed, %dv \n",
				__func__,smb1357data->pogo_status);
			smb1357_syson_sensing(client, 10, false);
			smb1357_dcin_config_hv_lv(client, true);
			smb1357data->pogo_status = DCIN_NONE;
			if (smb1357data->usbin_cable_type == POWER_SUPPLY_TYPE_BATTERY) {
				val.intval = POWER_SUPPLY_TYPE_BATTERY;
				psy_do_property("battery",set,
					POWER_SUPPLY_PROP_ONLINE, val);
				msleep(100);
			}
			else {
				/* Connected USBIN port, Just update UI */
				if (smb1357data->psy_bat) {
					power_supply_changed(smb1357data->psy_bat);
					dev_dbg(&client->dev,"%s,Just update UI\n",
						__func__);
				}
			}
			wake_unlock(&smb1357data->chg_wake_lock);
		}
	}

	dev_dbg(&client->dev," %s, det [ %d ], mode [ %d v]",
		__func__, gpio, smb1357data->pogo_status);
}

static void smb1357_charger_init_work(struct work_struct *work)
{
	struct smb1357_charger_data *smb1357data =
	    container_of(work, struct smb1357_charger_data,init_work.work);
	struct i2c_client *client = smb1357data->charger->client;
	struct sec_battery_info *battery;
	int ret = 0;
	smb1357data->psy_bat = power_supply_get_by_name("battery");

	if (smb1357data->psy_bat != NULL) {
		battery =
		container_of(smb1357data->psy_bat, struct sec_battery_info, psy_bat);

		smb1357data->psy_pogo.name = "pogo",
		smb1357data->psy_pogo.type = POWER_SUPPLY_TYPE_POGODOCK,
		smb1357data->psy_pogo.supplied_to = pogo_supply_list,
		smb1357data->psy_pogo.num_supplicants = ARRAY_SIZE(pogo_supply_list);
		smb1357data->psy_pogo.properties = pogo_power_props,
		smb1357data->psy_pogo.num_properties = ARRAY_SIZE(pogo_power_props),
		smb1357data->psy_pogo.get_property = sec_pogo_get_property;

		ret = power_supply_register(battery->dev, &smb1357data->psy_pogo);
		if (ret) {
			dev_err(battery->dev,
				"%s: Failed to Register psy_pogo\n", __func__);
		}
	}

	smb1357_dcin_config_hv_lv(client, false);

	if (smb1357_is_pogo_event(client)) {
		if (smb1357data->pogo_det_gpio)	{
			wake_lock(&smb1357data->chg_wake_lock);
			cancel_delayed_work(&smb1357data->pogo_det_work);
			smb1357data->pogo_det_count = 3;
			schedule_delayed_work(&smb1357data->pogo_det_work,
				msecs_to_jiffies(200));
		}
	}

#if defined(ENABLE_MAX77804K_CHG_IRQ)
	if (max77804k_tiny_irq_batp > 0) {
		smb1357data->detbat_irq = max77804k_tiny_irq_batp;
		if (smb1357data->detbat_irq){
			INIT_DELAYED_WORK(
				&smb1357data->detbat_work, smb1357_detbat_work);
			ret = request_threaded_irq(smb1357data->detbat_irq,
				NULL, smb1357_detbat_irq_thread, IRQF_TRIGGER_FALLING,
				"detbat-irq", smb1357data->charger);
			if (ret) {
				dev_err(&client->dev,
					"%s: Failed to Reqeust DETBAT IRQ\n", __func__);
			}
		}
	}

	dev_dbg(&client->dev,"%s batp %d\n", __func__, max77804k_tiny_irq_batp);
#endif
	return;
}

#if defined(USE_DEBUG_WORK)
static void smb1357_charger_debug_work(struct work_struct *work)
{
	struct smb1357_charger_data *smb1357data =
	    container_of(work, struct smb1357_charger_data,debug_work.work);
	struct i2c_client *client = smb1357data->charger->client;

	u8 buf_a = 0;
	u8 buf_b = 0;
	u8 addr = 0;

	for (addr = 0; addr <= 0x1f; addr++)
	{
		smb1357_charger_i2c_read(client, addr, &buf_a);
		smb1357_charger_i2c_read(client, addr+0x40, &buf_b);
		dev_dbg(&client->dev,"[reg 0x%2x : 0x%2x], [reg 0x%2x : 0x%2x]\n",
				addr, buf_a, addr+0x40, buf_b);
	}
	dev_dbg(&client->dev,"============================================\n");
	schedule_delayed_work(&smb1357data->debug_work, msecs_to_jiffies(10000));
	return;
}
#endif

#if defined(ENABLE_EXTERNAL_APSD)
extern void smb1357_charger_external_apsd(u8 enable)
{
	struct power_supply *psy;
	struct sec_charger_info *charger;
	struct smb1357_charger_data *smb1357data;
	struct i2c_client *client;
	u8 apsd_reg = 0x00;

	psy = power_supply_get_by_name("smb1357-charger");

	if (psy == NULL) {
		pr_err("%s smb1357 loading failed.\n", __func__);
		goto out;
	}

	charger = container_of(psy, struct sec_charger_info, psy_chg);
	smb1357data = dev_get_drvdata(&charger->client->dev);
	client = smb1357data->charger->client;

	smb1357_charger_i2c_read(client, CFG_11_REG, &apsd_reg);
	apsd_reg &= AUTO_DET_SRC_EN_BIT;

	if (apsd_reg == enable)	{
		pr_err("%s apsd_reg = [0x%x].\n", __func__, apsd_reg);
		goto shdn;
	}

	if (enable) {
		smb1357_write_enable(client, true);
		msleep(10);

		/* USBIN ADAPTER : 9V only */
		smb1357_charger_masked_write_reg(client, CFG_C_REG,
			USBIN_ADAPTER_MASK, 0x60);

		smb1357_set_APSD(client, true);

		/* USBIN ADAPTER : 5V or 9V */
		smb1357_charger_masked_write_reg(client, CFG_C_REG,
			USBIN_ADAPTER_MASK, 0x20);
	}
	dev_dbg(&client->dev,"%s enable [%d]\n", __func__, enable);
shdn:
	smb1357_chgin_shutdown(client, true);
out:
	return;
}
#endif

static int smb1357_charger_probe(
						struct i2c_client *client,
						const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter =
		to_i2c_adapter(client->dev.parent);
	struct smb1357_charger_data *smb1357data;
	struct sec_charger_info *charger;
	int ret = 0;

	dev_info(&client->dev,
		"%s: SMB1357 Charger Driver Loading\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	charger = kzalloc(sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

	smb1357data = kzalloc(sizeof(*smb1357data), GFP_KERNEL);
	if (!smb1357data){
		ret = -ENOMEM;
		goto err_free2;
	}

	smb1357data->charger = charger;
	smb1357data->chg_limit = false;
	smb1357data->pogo_status = DCIN_NONE;
	smb1357data->usbin_cable_type = POWER_SUPPLY_TYPE_BATTERY;
	smb1357data->charger->cable_type = POWER_SUPPLY_TYPE_BATTERY;
	mutex_init(&smb1357data->mutex);

	chg_temp_table_size =
		sizeof(chg_temp_table)/sizeof(sec_bat_adc_table_data_t);

	smb1357data->siop_level = 100;
	charger->client = client;

	if (client->dev.of_node) {
		void * pdata = kzalloc(sizeof(sec_battery_platform_data_t), GFP_KERNEL);
		if (!pdata)
			goto err_free1;
		charger->pdata = pdata;
		if (smb1357_charger_parse_dt(smb1357data))
			dev_err(&client->dev,
				"%s : Failed to get charger dt\n", __func__);
	} else
		charger->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, smb1357data);

	charger->psy_chg.name		= DEV_CHG_NAME;
	charger->psy_chg.type		= POWER_SUPPLY_TYPE_UNKNOWN;
	charger->psy_chg.get_property	= smb1357_chg_get_property;
	charger->psy_chg.set_property	= smb1357_chg_set_property;
	charger->psy_chg.properties	= smb1357_charger_props;
	charger->psy_chg.num_properties	= ARRAY_SIZE(smb1357_charger_props);

	if (charger->pdata->chg_gpio_init) {
		if (!charger->pdata->chg_gpio_init()) {
			dev_err(&client->dev,
					"%s: Failed to Initialize GPIO\n", __func__);
			goto err_free;
		}
	}

	smb1357_check_product_id(charger->client);

	/* volatile access */
	smb1357_write_enable(charger->client, true);
	msleep(10);

	smb1357_gpio_init(smb1357data);

	smb1357_hw_init(charger->client);

	if (!smb1357_hal_chg_init(charger->client)) {
		dev_err(&client->dev,
			"%s: Failed to Initialize Charger\n", __func__);
		goto err_free;
	}

	ret = power_supply_register(&client->dev, &charger->psy_chg);
	if (ret) {
		dev_err(&client->dev,
			"%s: Failed to Register psy_chg\n", __func__);
		goto err_free;
	}

	if (charger->pdata->chg_irq) {
		INIT_DELAYED_WORK(&charger->isr_work, smb1357_chg_isr_work);

		ret = request_threaded_irq(charger->pdata->chg_irq,
				NULL, smb1357_chg_irq_thread,
				charger->pdata->chg_irq_attr,
				"smbcharger_irq", charger);
		if (ret) {
			dev_err(&client->dev,
				"%s: Failed to Reqeust IRQ\n", __func__);
			goto err_supply_unreg;
		}

		ret = enable_irq_wake(charger->pdata->chg_irq);
		if (ret < 0)
			dev_err(&client->dev,
				"%s: Failed to Enable Wakeup Source(%d)\n",
				__func__, ret);
	}

	ret = smb1357_chg_create_attrs(charger->psy_chg.dev);
	if (ret) {
		dev_err(&client->dev,
			"%s : Failed to create_attrs\n", __func__);
		goto err_req_irq;
	}

	wake_lock_init(&smb1357data->chg_wake_lock, WAKE_LOCK_SUSPEND,
					"chg-wakelock");
	wake_lock_init(&smb1357data->chgisr_wake_lock, WAKE_LOCK_SUSPEND,
					"chgisr-wakelock");
	INIT_DELAYED_WORK(&smb1357data->hvdcp_det_work,
				smb1357_charger_hvdcp_det_work);
	INIT_DELAYED_WORK(&smb1357data->pogo_det_work,
				smb1357_charger_pogo_det_work);
	INIT_DELAYED_WORK(&smb1357data->syson_trigger_work,
				smb1357_syson_trigger_work);
	INIT_DELAYED_WORK(&smb1357data->init_work,
				smb1357_charger_init_work);
	schedule_delayed_work(&smb1357data->init_work, msecs_to_jiffies(3000));
#if defined(USE_DEBUG_WORK)
	INIT_DELAYED_WORK(&smb1357data->debug_work, smb1357_charger_debug_work);
	schedule_delayed_work(&smb1357data->debug_work, msecs_to_jiffies(5000));
#endif

#if defined(TIMER_FORCE_BLOCK)
	force_block = false;
	force_block_count = 0;
#endif

	dev_dbg(&client->dev,
		"%s: SMB1357 Charger Driver Loaded\n", __func__);
	return 0;

err_req_irq:
	if (charger->pdata->chg_irq)
		free_irq(charger->pdata->chg_irq, charger);
err_supply_unreg:
	power_supply_unregister(&charger->psy_chg);
err_free:
	kfree(charger->pdata);
err_free1:
	mutex_destroy(&smb1357data->mutex);
	kfree(smb1357data);
err_free2:
	kfree(charger);
	return ret;
}

static int smb1357_charger_remove(struct i2c_client *client)
{
	return 0;
}

static int smb1357_charger_suspend(struct device *dev)
{
	struct smb1357_charger_data *smb1357data = dev_get_drvdata(dev);
	struct sec_charger_info *charger = smb1357data->charger;

	if (!smb1357_hal_chg_suspend(charger->client))
		dev_err(&charger->client->dev,
			"%s: Failed to Suspend Charger\n", __func__);

	dev_info(&charger->client->dev,"%s: stop \n", __func__);

	return 0;
}

static int smb1357_charger_resume(struct device *dev)
{
	struct smb1357_charger_data *smb1357data = dev_get_drvdata(dev);
	struct sec_charger_info *charger = smb1357data->charger;

	dev_info(&charger->client->dev,"%s: start\n", __func__);

	if (!smb1357_hal_chg_resume(charger->client))
		dev_err(&charger->client->dev,
			"%s: Failed to Resume Charger\n", __func__);

	return 0;
}

static const struct dev_pm_ops smb1357_charger_pm_ops = {
	.suspend = smb1357_charger_suspend,
	.resume  = smb1357_charger_resume,
};

static const struct i2c_device_id smb1357_charger_id[] = {
	{"smb1357-charger", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, smb1357_charger_id);
static struct of_device_id smb1357_i2c_match_table[] = {
	{ .compatible = "smb1357-charger,i2c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, smb1357_i2c_match_table);

static struct i2c_driver smb1357_charger_driver = {
	.driver = {
		.name	= "smb1357-charger",
		.owner = THIS_MODULE,
		.of_match_table = smb1357_i2c_match_table,
#ifdef CONFIG_PM
		.pm = &smb1357_charger_pm_ops,
#endif
	},
	.probe	= smb1357_charger_probe,
	.remove	= smb1357_charger_remove,
	.shutdown	= smb1357_charger_shutdown,
	.id_table	= smb1357_charger_id,
};

static int __init smb1357_charger_init(void)
{
	return i2c_add_driver(&smb1357_charger_driver);
}

static void __exit smb1357_charger_exit(void)
{
	i2c_del_driver(&smb1357_charger_driver);
}

module_init(smb1357_charger_init);
module_exit(smb1357_charger_exit);

#if defined(ENABLE_MAX77804K_CHG_IRQ)
static void max77804k_tiny_init_work(struct work_struct *work)
{
	struct max77804k_tiny_charger_data *max77804k_tiny_charger =
		container_of(work, struct max77804k_tiny_charger_data,
					tiny_init_work.work);
	u8 reg_data;

#ifdef CONFIG_SEC_FACTORY
	reg_data = 0xFF;
	max77804k_update_reg(max77804k_tiny_charger->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, reg_data, 0xFF);
#else
	reg_data = 0x00;
	max77804k_update_reg(max77804k_tiny_charger->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, reg_data, 0x04);
#endif

	pr_info("%s\n",__func__);
	return;
}

static int max77804k_tiny_charger_probe(struct platform_device *pdev)
{
	struct max77804k_dev *iodev = dev_get_drvdata(pdev->dev.parent);
	struct max77804k_tiny_charger_data *max77804k_tiny_charger;
	int ret = 0;

	pr_info("%s : MAX77804K Tiny Charger Driver\n",__func__);

	max77804k_tiny_charger =
			kzalloc(sizeof(*max77804k_tiny_charger), GFP_KERNEL);

	if (!max77804k_tiny_charger){
		ret = -ENOMEM;
		goto err_free;
	}

	max77804k_tiny_charger->max77804k = iodev;

	platform_set_drvdata(pdev, max77804k_tiny_charger);

	INIT_DELAYED_WORK(&max77804k_tiny_charger->tiny_init_work,
		max77804k_tiny_init_work);

#ifndef CONFIG_SEC_FACTORY
	if (max77804k_tiny_charger->max77804k->irq_base) {
		max77804k_tiny_irq_batp =
		max77804k_tiny_charger->max77804k->irq_base + MAX77804K_CHG_IRQ_BATP_I;
	}
	else
#endif
		max77804k_tiny_irq_batp = 0;

	schedule_delayed_work(&max77804k_tiny_charger->tiny_init_work,
					msecs_to_jiffies(3000));
err_free:
	return ret;
}

static void max77804k_tiny_charger_shutdown(struct device *dev)
{
	struct max77804k_tiny_charger_data *max77804k_tiny_charger =
			dev_get_drvdata(dev);

	u8 reg_data;
	pr_info("%s MAX77804K Tiny Driver Shutdown\n",__func__);

	reg_data = 0xff;
	max77804k_write_reg(max77804k_tiny_charger->max77804k->i2c,
			MAX77804K_CHG_REG_CHG_INT_MASK, reg_data);

	return;
}

static int max77804k_tiny_charger_remove(struct platform_device *pdev)
{
	struct max77804k_tiny_charger_data *max77804k_tiny_charger =
			platform_get_drvdata(pdev);
	pr_info("%s \n",__func__);

	if(max77804k_tiny_charger)
		kfree(max77804k_tiny_charger);

	return 0;
}

static struct platform_driver max77804k_tiny_charger_driver = {
	.driver     = {
		.name   = "max77804k-charger",
		.owner  = THIS_MODULE,
		.shutdown = max77804k_tiny_charger_shutdown,
	},
	.probe      = max77804k_tiny_charger_probe,
	.remove     = max77804k_tiny_charger_remove,
};

static int __init max77804k_tiny_charger_init(void)
{
	pr_info("func:%s\n", __func__);
	return platform_driver_register(&max77804k_tiny_charger_driver);
}

static void __exit max77804k_tiny_charger_exit(void)
{
	pr_info("func:%s\n", __func__);
	platform_driver_unregister(&max77804k_tiny_charger_driver);
}

module_init(max77804k_tiny_charger_init);
module_exit(max77804k_tiny_charger_exit);
#endif

MODULE_DESCRIPTION("Samsung SMB1357 Charger Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
