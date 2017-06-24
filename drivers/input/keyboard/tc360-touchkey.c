/*
 * CORERIVER TOUCHCORE touchkey driver
 *
 * Copyright (C) 2012 Samsung Electronics Co.Ltd
 * Author: Taeyoon Yoon <tyoony.yoon@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

//#define LED_DEBUG
#define ISP_DEBUG
//#define ISP_VERBOSE_DEBUG
//#define ISP_VERY_VERBOSE_DEBUG

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/firmware.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/tc360-touchkey.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>

#define TC300K_FW_NAME "tc300k_chagall"

#define USE_OPEN_CLOSE

#define TC300K_FW_BUILTIN_PATH	"tkey"
#define TC300K_FW_IN_SDCARD_PATH	"/sdcard/"

#define TC300K_POWERON_DELAY	300

#define TC300K_KEY_INDEX_MASK	0x07
#define TC300K_KEY_PRESS_MASK	0x08

/* registers */
#define TC300K_KEYCODE		0x00
#define TC300K_FWVER		0x01
#define TC300K_MDVER		0x02
#define TC300K_MODE			0x03
#define TC300K_CHECKS_H		0x04
#define TC300K_CHECKS_L		0x05
#define TC300K_THRES_H		0x06
#define TC300K_THRES_L		0x07
#define TC300K_1KEY_DATA	0x08
#define TC300K_2KEY_DATA	0x0E
#define TC300K_3KEY_DATA	0x14
#define TC300K_4KEY_DATA	0x1A
#define TC300K_5KEY_DATA	0x20
#define TC300K_6KEY_DATA	0x26

#define TC300K_CH_PCK_H_OFFSET	0x00
#define TC300K_CH_PCK_L_OFFSET	0x01
#define TC300K_DIFF_H_OFFSET	0x02
#define TC300K_DIFF_L_OFFSET	0x03
#define TC300K_RAW_H_OFFSET		0x04
#define TC300K_RAW_L_OFFSET		0x05

/* command */
#define TC300K_CMD_ADDR			0x00
#define TC300K_CMD_LED_ON		0x10
#define TC300K_CMD_LED_OFF		0x20
#define TC300K_CMD_GLOVE_ON		0x30
#define TC300K_CMD_GLOVE_OFF	0x40
#define TC300K_CMD_FAC_ON		0x50
#define TC300K_CMD_FAC_OFF		0x60
#define TC300K_CMD_CAL_CHECKSUM	0x70
#define TC300K_CMD_DELAY		50

/* ISP command */
#define TC300K_CSYNC1			0xA3
#define TC300K_CSYNC2			0xAC
#define TC300K_CSYNC3			0xA5
#define TC300K_CCFG				0x92
#define TC300K_PRDATA			0x81
#define TC300K_PEDATA			0x82
#define TC300K_PWDATA			0x83
#define TC300K_PECHIP			0x8A
#define TC300K_PEDISC			0xB0
#define TC300K_LDDATA			0xB2
#define TC300K_LDMODE			0xB8
#define TC300K_RDDATA			0xB9
#define TC300K_PCRST			0xB4
#define TC300K_PCRED			0xB5
#define TC300K_PCINC			0xB6
#define TC300K_RDPCH			0xBD

/* ISP delay */
#define TC300K_TSYNC1			300	/* us */
#define TC300K_TSYNC2			50	/* 1ms~50ms */
#define TC300K_TSYNC3			100	/* us */
#define TC300K_TDLY1			1	/* us */
#define TC300K_TDLY2			2	/* us */
#define TC300K_TFERASE			10	/* ms */
#define TC300K_TPROG			20	/* us */

#define TC300K_CHECKSUM_DELAY	500
#define TO_STRING(x) #x

enum {
	FW_BUILT_IN = 0,
	FW_IN_SDCARD,
};

enum {
	HAVE_LATEST_FW = 1,
	FW_UPDATE_RUNNING,
};

enum {
	DOWNLOADING = 1,
	FAIL,
	PASS,
};

struct fdata_struct {
	struct device			*dummy_dev;
	u8				fw_flash_status;
	u8				fw_update_skip;
};

struct fw_image {
	u8 hdr_ver;
	u8 hdr_len;
	u16 first_fw_ver;
	u16 second_fw_ver;
	u16 third_ver;
	u32 fw_len;
	u16 checksum;
	u16 alignment_dummy;
	u8 data[0];
} __attribute__ ((packed));

struct tc300k_data {
	struct i2c_client		*client;
	struct input_dev		*input_dev;
	char				phys[32];
	struct tc300k_platform_data	*pdata;
	struct early_suspend		early_suspend;
	struct mutex			lock;
	struct fw_image			*fw_img;
	u8		suspend_type;
	u32		scl;
	u32		sda;
	u16 	threhold;
	int		num_key;
	int		*keycodes;
	bool	enabled;
	bool	glove_mode;
	bool	factory_mode;
	bool	led_on;
	/* variables for fw update*/
	struct fdata_struct		*fdata;
	const struct firmware		*fw;
	u8				cur_fw_path;
	struct workqueue_struct		*fw_wq;
	struct work_struct		fw_work;
	struct wake_lock		fw_wake_lock;
	u8				fw_flash_state;
	bool				fw_up_running;

	/* variables for LED*/
	struct led_classdev		led;
	struct workqueue_struct		*led_wq;
	struct work_struct		led_work;
	u8				led_brightness;
	bool				counting_timer;
	struct regulator *vcc_en;
	struct regulator *vdd_led;

	/*variable for switching*/
	int led_wq_passed;
	int ic_fw_ver;

#ifdef TSP_BOOSTER
	struct delayed_work	work_dvfs_off;
	struct delayed_work	work_dvfs_chg;
	struct mutex		dvfs_lock;
	bool dvfs_lock_status;
	int dvfs_old_stauts;
	int dvfs_boost_mode;
	int dvfs_freq;
#endif
};

static void tc300k_destroy_interface(struct tc300k_data *data);
#ifdef CONFIG_HAS_EARLYSUSPEND
static void tc300k_early_suspend(struct early_suspend *h);
static void tc300k_late_resume(struct early_suspend *h);
#endif 
#ifdef USE_OPEN_CLOSE
static int tc300k_input_open(struct input_dev *dev);
static void tc300k_input_close(struct input_dev *dev);
#endif

static int tc300k_keycodes[]={KEY_BACK,KEY_RECENT,KEY_DUMMY_BACK,KEY_DUMMY_MENU};


#ifdef TSP_BOOSTER
static void tc300k_change_dvfs_lock(struct work_struct *work)
{
	struct tc300k_data *data = container_of(work, struct tc300k_data, work_dvfs_chg.work);
	int retval = 0;
	mutex_lock(&data->dvfs_lock);

	retval = set_freq_limit(DVFS_TOUCH_ID, data->dvfs_freq);
	if (retval < 0)
		dev_info(&data->client->dev,
			"%s: booster change failed(%d).\n",
			__func__, retval);
	data->dvfs_lock_status = false;
	mutex_unlock(&data->dvfs_lock);
}

static void tc300k_set_dvfs_off(struct work_struct *work)
{
	struct tc300k_data *data =
		container_of(work, struct tc300k_data, work_dvfs_off.work);
	int retval;

	mutex_lock(&data->dvfs_lock);
	retval = set_freq_limit(DVFS_TOUCH_ID, -1);
	if (retval < 0)
		dev_info(&data->client->dev,
			"%s: booster stop failed(%d).\n",
			__func__, retval);

	data->dvfs_lock_status = true;
	mutex_unlock(&data->dvfs_lock);
}

static void tc300k_set_dvfs_lock(struct tc300k_data *data,
					uint32_t on)
{
	int ret = 0;
	
	if (data->dvfs_boost_mode == DVFS_STAGE_NONE) {
		dev_dbg(&data->client->dev,
				"%s: DVFS stage is none(%d)\n",
				__func__, data->dvfs_boost_mode);
		return;
	}

	mutex_lock(&data->dvfs_lock);
	if (on == 1) {
			cancel_delayed_work(&data->work_dvfs_chg);

		if (data->dvfs_lock_status) {
			ret = set_freq_limit(DVFS_TOUCH_ID, data->dvfs_freq);
					if (ret < 0)
						dev_info(&data->client->dev,
					"%s: cpu first lock failed(%d)\n", __func__, ret);
			data->dvfs_lock_status = false;
		}

		schedule_delayed_work(&data->work_dvfs_off,
			msecs_to_jiffies(TOUCH_BOOSTER_CHG_TIME));

	} else if (on == 0) {
		cancel_delayed_work(&data->work_dvfs_off);
				schedule_delayed_work(&data->work_dvfs_chg,
				msecs_to_jiffies(TOUCH_BOOSTER_OFF_TIME));

	} else if (on == 2) {
		if (data->dvfs_lock_status) {
			cancel_delayed_work(&data->work_dvfs_off);
			cancel_delayed_work(&data->work_dvfs_chg);
			schedule_work(&data->work_dvfs_off.work);
		}
	}
	mutex_unlock(&data->dvfs_lock);
}

static void tc300k_init_dvfs(struct tc300k_data *data)
{
	mutex_init(&data->dvfs_lock);
	data->dvfs_boost_mode = DVFS_STAGE_DUAL;
	data->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;

	INIT_DELAYED_WORK(&data->work_dvfs_off, tc300k_set_dvfs_off);
	INIT_DELAYED_WORK(&data->work_dvfs_chg, tc300k_change_dvfs_lock);

	data->dvfs_lock_status = true;
}
#endif

static void tc300k_gpio_request(struct tc300k_data *data)
{
	int ret = 0;
	dev_info(&data->client->dev, "%s: enter \n",__func__);

	ret = gpio_request(data->pdata->gpio_en, "touchkey_en_gpio");
			if (ret) {
				printk(KERN_ERR "%s: unable to request touchkey_en_gpio[%d]\n",
						__func__, data->pdata->gpio_en);
			}

	ret = gpio_request(data->pdata->gpio_int, "touchkey_irq");
		if (ret) {
			printk(KERN_ERR "%s: unable to request touchkey_irq [%d]\n",
					__func__, data->pdata->gpio_int);
		}

	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

}

void tc300k_power(struct tc300k_data *data, int onoff)
{
	int ret = 0;

	dev_info(&data->client->dev, "%s: power %s\n",__func__, onoff ? "on" : "off");

	if ((s32)data->pdata->gpio_en > 0) {
		ret = gpio_direction_output(data->pdata->gpio_en, onoff);
		if (ret) {
			dev_err(&data->client->dev,
					"%s: unable to set_direction for gpio_en [%d]\n",
					__func__, data->pdata->gpio_en);
		}
	}
	else{
		//check for SUB PMIC ldo
		if (!data->vcc_en) {
			if (data->pdata->vcc_en_ldo_name){
				data->vcc_en = regulator_get(NULL, data->pdata->vcc_en_ldo_name);
				if (IS_ERR(data->vcc_en)) {
					dev_err(&data->client->dev,"Regulator(vcc_en) get failed rc = %ld\n", PTR_ERR(data->vcc_en));
					return;
				}
				ret = regulator_set_voltage(data->vcc_en,1800000, 1800000);
				if (ret) {
					dev_err(&data->client->dev,"regulator(vcc_en) set_vtg failed rc=%d\n", ret);
					return;
				}
			} else {
				dev_err(&data->client->dev,"vcc_en_ldo_name is not read from dtsi");
				return;
			}
		}

		if (onoff) {
			if (!regulator_is_enabled(data->vcc_en)) {
				ret = regulator_enable(data->vcc_en);
				if (ret) {
					pr_err("%s: enable vcc_en failed, rc=%d\n",__func__, ret);
					return;
				}
				pr_info("%senable vcc_en success.\n",__func__);
			} else {
				pr_info("%svcc_en is already enabled.\n",__func__);
			}
		} else {
			if (regulator_is_enabled(data->vcc_en)) {
				ret = regulator_disable(data->vcc_en);
				if (ret) {
					pr_err("%s: disable vcc_en failed, rc=%d\n", __func__, ret);
					return;
				}
				pr_info("%sdisable vcc_en success.\n",__func__);
			} else {
				pr_info("%svcc_en is already disabled.\n",__func__);
			}
		}
	}
}

void tc300k_led_power(struct tc300k_data *data, bool onoff)
{
	int ret;

	pr_info("%s: %s\n", __func__, onoff ? "on" : "off");

	if (!data->vdd_led) {
		data->vdd_led = regulator_get(&data->client->dev,"vdd_led");
		if (IS_ERR(data->vdd_led)) {
			dev_err(&data->client->dev,"Regulator(vdd_led) get failed for PMIC. rc = %ld\n", PTR_ERR(data->vdd_led));

			//check for SUB PMIC ldo
			if (data->pdata->vdd_led_ldo_name){
                                data->vdd_led = regulator_get(NULL, data->pdata->vdd_led_ldo_name);
                                if (IS_ERR(data->vdd_led)) {
                                        dev_err(&data->client->dev,"Regulator(vdd_led) get failed for SUB PMIC. rc = %ld\n", PTR_ERR(data->vdd_led));
                                        return;
                                }
                        } else {
                                dev_err(&data->client->dev,"vdd_led_ldo_name is not read from dtsi");
                                return;
                        }
		}
		ret = regulator_set_voltage(data->vdd_led,3300000, 3300000);
		if (ret) {
			dev_err(&data->client->dev,
				"regulator(vdd_led) set_vtg failed rc=%d\n", ret);
			return;
		}
	}
	
	if (onoff) {
		if (!regulator_is_enabled(data->vdd_led)) {
			ret = regulator_enable(data->vdd_led);
			if (ret) {
				pr_err("%s: enable vdd_led failed, rc=%d\n",__func__, ret);
				return;
			}
			pr_info("%sTKEY_LED 3.3V[vdd_led] on is finished.\n",__func__);			
		} else
			pr_info("%sTKEY_LED 3.3V[vdd_led] is already on.\n",__func__);
	} else {
		if (regulator_is_enabled(data->vdd_led)) {
			ret = regulator_disable(data->vdd_led);
			if (ret) {
				pr_err("%s: disable vdd_led failed, rc=%d\n",
					__func__, ret);
				return;
			}
			pr_info("%sTKEY_LED 3.3V[vdd_led] off is finished.\n",__func__);
		} else
			pr_info("%sTKEY_LED 3.3V[vdd_led] is already off.\n",__func__);	
	}
}

#ifdef CONFIG_OF
static int tc300k_parse_dt(struct device *dev,
			struct tc300k_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int rc;

	pdata->num_key =ARRAY_SIZE(tc300k_keycodes);
	pdata->keycodes = tc300k_keycodes;

	pdata->gpio_en = of_get_named_gpio_flags(np, "coreriver,vcc_en-gpio", 0, &pdata->vcc_gpio_flags);
	pdata->gpio_scl = of_get_named_gpio_flags(np, "coreriver,scl-gpio", 0, &pdata->scl_gpio_flags);
	pdata->gpio_sda = of_get_named_gpio_flags(np, "coreriver,sda-gpio", 0, &pdata->sda_gpio_flags);
	pdata->gpio_int = of_get_named_gpio_flags(np, "coreriver,irq-gpio", 0, &pdata->irq_gpio_flags);

	//raed SUB PMIC ldo names from dtsi
	rc = of_property_read_string(np, "coreriver,vcc_en_ldo_name", &pdata->vcc_en_ldo_name);
	if (rc < 0) {
                pr_err("[%s]: Unable to read vcc_en_ldo_name rc = %d\n", __func__, rc);
        }
        rc = of_property_read_string(np, "coreriver,vdd_led_ldo_name", &pdata->vdd_led_ldo_name);
        if (rc < 0) {
                pr_err("[%s]: Unable to read vdd_led_ldo_name rc = %d\n", __func__, rc);
        }
	
	pr_err("%s: gpio_en = %d, tkey_scl= %d, tkey_sda= %d, tkey_int= %d",
				__func__, pdata->gpio_en, pdata->gpio_scl, pdata->gpio_sda, pdata->gpio_int);
	
	return 0;
}
#else
static int tc300k_parse_dt(struct device *dev,
			struct tc300k_platform_data *pdata)
{
	return -ENODEV;
}
#endif

static irqreturn_t tc300k_interrupt(int irq, void *dev_id)
{
	struct tc300k_data *data = dev_id;
	struct i2c_client *client = data->client;
	u8 key_val, index;
	bool press;
	int ret;

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return IRQ_HANDLED;
	}
	
	ret = i2c_smbus_read_byte_data(client, TC300K_KEYCODE);
	if (ret < 0) {
		dev_err(&client->dev, "failed to read key data (%d)\n", ret);
		return IRQ_HANDLED;
	}
	
	key_val = (u8)ret;
	index = key_val & TC300K_KEY_INDEX_MASK;
	press = !!(key_val & TC300K_KEY_PRESS_MASK);
	
	
	if (press) {
		input_report_key(data->input_dev, data->keycodes[index-1], 0);

#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
			dev_notice(&client->dev, "key R\n");
#else
			dev_notice(&client->dev,
				"key R : %d(%d)\n", data->keycodes[index-1], key_val);
#endif
	} else {
		input_report_key(data->input_dev, data->keycodes[index-1], 1);
#ifdef CONFIG_SAMSUNG_PRODUCT_SHIP
			dev_notice(&client->dev, "key P\n");
#else
			dev_notice(&client->dev,
				"key P : %d(%d)\n", data->keycodes[index-1], key_val);
#endif
	}
	input_sync(data->input_dev);

#ifdef TSP_BOOSTER
		tc300k_set_dvfs_lock(data, !!press);
#endif

	return IRQ_HANDLED;
}

static int tc300k_get_module_ver(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int retries = 3;
	int module_ver;

read_module_version:
	module_ver = i2c_smbus_read_byte_data(client, TC300K_MDVER);
	if (module_ver < 0) {
		dev_err(&client->dev, "failed to read module ver (%d)\n", module_ver);
		if (retries-- > 0) {
			tc300k_power(data,0);
			msleep(TC300K_POWERON_DELAY);
			tc300k_power(data,1);
			msleep(TC300K_POWERON_DELAY);
			goto read_module_version;
		}
	}

	return module_ver;
}

static int tc300k_get_fw_ver(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ver;
	int retries = 2;
read_version:
	ver = i2c_smbus_read_byte_data(client, TC300K_FWVER);
	if (ver < 0) {
		dev_err(&client->dev, "tc300k_get_fw_ver : failed to read fw ver (%d)\n",ver);
		if (retries-- > 0) {
			tc300k_power(data,0);
			msleep(TC300K_POWERON_DELAY);
			tc300k_power(data,1);
			msleep(TC300K_POWERON_DELAY);
			goto read_version;
		}
	}
	else
		data->ic_fw_ver= ver;

	return ver;
}
static int load_fw_built_in(struct tc300k_data *data);


static inline void setsda(struct tc300k_data *data, int state)
{
	if (state)
		gpio_direction_output(data->pdata->gpio_sda, 1);
	else
		gpio_direction_output(data->pdata->gpio_sda, 0);
}

static inline void setscl(struct tc300k_data *data, int state)
{
	if (state)
		gpio_direction_output(data->pdata->gpio_scl, 1);
	else
		gpio_direction_output(data->pdata->gpio_scl, 0);
}

static inline int getsda(struct tc300k_data *data)
{
	return gpio_get_value(data->pdata->gpio_sda);
}

static inline int getscl(struct tc300k_data *data)
{
	return gpio_get_value(data->pdata->gpio_scl);
}

static void send_9bit(struct tc300k_data *data, u8 buff)
{
	int i;

	setscl(data, 1);
	setsda(data, 0);
	setscl(data, 0);

	for (i = 0; i < 8; i++) {
		setscl(data, 1);
		setsda(data, (buff >> i) & 0x01);
		setscl(data, 0);
	}

	setsda(data, 0);
}

static u8 wait_9bit(struct tc300k_data *data)
{
	int i;
	int buf;
	u8 send_buf = 0;

	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_sda,0, GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		
	getsda(data);
	setscl(data, 1);
	setscl(data, 0);

	for (i = 0; i < 8; i++) {
		setscl(data, 1);
		buf = getsda(data);
		setscl(data, 0);
		send_buf |= (buf & 0x01) << i;
	}
	setsda(data, 0);

	return send_buf;
}

static void tc300k_reset_for_isp(struct tc300k_data *data, bool start)
{
	if (start) {
		tc300k_led_power(data,0);

		gpio_direction_input(data->pdata->gpio_int);
		gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_int,0, GPIO_CFG_INPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_scl,0, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
		gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_sda,0, GPIO_CFG_OUTPUT,GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);

		gpio_direction_output(data->pdata->gpio_scl, 0);
		gpio_direction_output(data->pdata->gpio_sda, 0);

		tc300k_power(data,0);
		msleep(100);

		tc300k_power(data,1);

		usleep_range(5000, 6000);

	} else {

		tc300k_led_power(data,0);
		tc300k_power(data,0);

		msleep(100);
		gpio_direction_output(data->pdata->gpio_scl, 1);
		gpio_direction_output(data->pdata->gpio_sda, 1);

		usleep(10);
		tc300k_power(data,1);
		msleep(70);
		tc300k_led_power(data,1);

		gpio_direction_input(data->pdata->gpio_sda);
		gpio_direction_input(data->pdata->gpio_scl);

		msleep(300);

	}
}

static void load(struct tc300k_data *data, u8 buff)
{
    send_9bit(data, TC300K_LDDATA);
    udelay(1);
    send_9bit(data, buff);
    udelay(1);
}

static void step(struct tc300k_data *data, u8 buff)
{
    send_9bit(data, TC300K_CCFG);
    udelay(1);
    send_9bit(data, buff);
    udelay(2);
}

static void setpc(struct tc300k_data *data, u16 addr)
{
    u8 buf[4];
    int i;

    buf[0] = 0x02;
    buf[1] = addr >> 8;
    buf[2] = addr & 0xff;
    buf[3] = 0x00;

    for (i = 0; i < 4; i++)
        step(data, buf[i]);
}

static void configure_isp(struct tc300k_data *data)
{
    u8 buf[7];
    int i;

    buf[0] = 0x75;    buf[1] = 0xFC;    buf[2] = 0xAC;
    buf[3] = 0x75;    buf[4] = 0xFC;    buf[5] = 0x35;
    buf[6] = 0x00;

    /* Step(cmd) */
    for (i = 0; i < 7; i++)
        step(data, buf[i]);
}

static int tc300k_erase_fw(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int i;
	u8 state = 0;

	tc300k_reset_for_isp(data, true);

	udelay(300);

	/* isp_enable_condition */
	send_9bit(data, TC300K_CSYNC1);
	udelay(9);
	send_9bit(data, TC300K_CSYNC2);
	udelay(9);
	send_9bit(data, TC300K_CSYNC3);
	usleep_range(150, 160);

	state = wait_9bit(data);
	if (state != 0x01) {
		dev_err(&client->dev, "%s isp enable error %d\n", __func__, state);
		return -1;
	}

	configure_isp(data);

	/* Full Chip Erase */
	send_9bit(data, TC300K_PCRST);
	udelay(1);
	send_9bit(data, TC300K_PECHIP);
	usleep_range(15000, 15500);


	state = 0;
	for (i = 0; i < 100; i++) {
		udelay(2);
		send_9bit(data, TC300K_CSYNC3);
		udelay(1);

		state = wait_9bit(data);
		if ((state & 0x04) == 0x00)
			break;
	}

	if (i == 100) {
		dev_err(&client->dev, "%s fail\n", __func__);
		return -1;
	}

	dev_info(&client->dev, "%s success\n", __func__);
	return 0;
}

static int tc300k_write_fw(struct tc300k_data *data)
{
//	struct i2c_client *client = data->client;
	u16 addr = 0;
	u8 code_data;

	setpc(data, addr);
	load(data, TC300K_PWDATA);
	send_9bit(data, TC300K_LDMODE);
	udelay(1);

	while (addr < data->fw_img->fw_len) {
		code_data = data->fw_img->data[addr++];
		load(data, code_data);
		usleep_range(20, 21);
	}

	send_9bit(data, TC300K_PEDISC);
	udelay(1);

	return 0;
}


static int tc300k_verify_fw(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	u16 addr = 0;
	u8 code_data;

	setpc(data, addr);

	dev_info(&client->dev, "fw code size = %#x (%u)",
		data->fw_img->fw_len, data->fw_img->fw_len);
	while (addr < data->fw_img->fw_len) {
		if ((addr % 0x40) == 0)
			dev_info(&client->dev, "fw verify addr = %#x\n", addr);

		send_9bit(data, TC300K_PRDATA);
		udelay(2);
		code_data = wait_9bit(data);
		udelay(1);

		if (code_data != data->fw_img->data[addr++]) {
			dev_err(&client->dev,
				"%s addr : %#x data error (0x%2x)\n",
				__func__, addr - 1, code_data );
			return -1;
		}
	}
	dev_info(&client->dev, "%s success\n", __func__);

	return 0;
}

static int tc300k_crc_check(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	u16 checksum;
	u8 cmd;
	u8 checksum_h, checksum_l;


	cmd = TC300K_CMD_CAL_CHECKSUM;
	ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, cmd);
	if (ret) {
		dev_err(&client->dev, "%s command fail (%d)\n", __func__, ret);
		return ret;
	}

	msleep(TC300K_CHECKSUM_DELAY);

	ret = i2c_smbus_read_byte_data(client, TC300K_CHECKS_H);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read checksum_h (%d)\n",
			__func__, ret);
		return ret;
	}
	checksum_h = ret;

	ret = i2c_smbus_read_byte_data(client, TC300K_CHECKS_L);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read checksum_l (%d)\n",
			__func__, ret);
		return ret;
	}
	checksum_l = ret;

	checksum = (checksum_h << 8) | checksum_l;

	if (data->fw_img->checksum != checksum) {
		dev_err(&client->dev,
			"%s checksum fail - firm checksum(%d), compute checksum(%d)\n",
			__func__, data->fw_img->checksum, checksum);
		return -1;
	}

	dev_info(&client->dev, "%s success (%d)\n", __func__, checksum);

	return 0;
}


static void tc300k_fw_update_worker(struct work_struct *work)
{
	struct tc300k_data *data = container_of(work, struct tc300k_data,
					       fw_work);
	struct i2c_client *client = data->client;
	int retries;
	int ret;
	int fw_ver;

	retries = 3;
	disable_irq(client->irq);
erase_fw:
	ret = tc300k_erase_fw(data);

	if (ret < 0) {
		dev_err(&client->dev, "fail to erase fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry erasing fw (%d)\n",
				 retries);
			goto erase_fw;
		} else {
			goto err_tc300k_flash_fw;
		}
	}

	dev_info(&client->dev, "succeed in erasing fw\n");

	retries = 3;

write_fw:
	/* Write */
	ret = tc300k_write_fw(data);
	if (ret < 0) {
			dev_err(&client->dev, "fail to write fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry writing fw (%d)\n",
				 retries);
			goto write_fw;
		} else {
			goto err_tc300k_flash_fw;
		}
	}
	dev_info(&client->dev, "succeed in writing fw\n");


	/* Verify */
	ret = tc300k_verify_fw(data);
	if (ret < 0) {
		dev_err(&client->dev, "fail to verify fw (%d)\n", ret);
		if (retries-- > 0) {
			dev_info(&client->dev, "retry verifing fw (%d)\n",
				 retries);
			goto erase_fw;
		} else {
			goto err_tc300k_flash_fw;
		}
	}
	dev_info(&client->dev, "succeed in verifing fw\n");
	
	tc300k_reset_for_isp(data, false);

	fw_ver = tc300k_get_fw_ver(data);
	dev_info(&client->dev, "fw_ver(%#x)\n", fw_ver);

	/* Crc check */
	ret = tc300k_crc_check(data);
	if (ret) {
		dev_err(&client->dev, "%s crc check fail (%d)\n",
				__func__, ret);
		goto	err_tc300k_flash_fw;
	}

	if (data->cur_fw_path == FW_BUILT_IN)
		release_firmware(data->fw);
	else if (data->cur_fw_path == FW_IN_SDCARD)
		kfree(data->fw_img);
	
	enable_irq(client->irq);
	data->enabled = true;
	data->fdata->fw_flash_status = PASS;

	return;

err_tc300k_flash_fw:
	dev_info(&client->dev, "failed to fw work\n");
	
}

static int load_fw_built_in(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	char *fw_name;

	fw_name = kasprintf(GFP_KERNEL, "%s/%s.fw",
		TC300K_FW_BUILTIN_PATH, TC300K_FW_NAME);

	pr_info("%s!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",fw_name);
	ret = request_firmware(&data->fw, fw_name, &client->dev);
	if (ret) {
		dev_err(&client->dev, "error requesting built-in firmware (%d)"
			"\n", ret);
		goto out;
	}

	data->fw_img = (struct fw_image *)data->fw->data;

	dev_info(&client->dev, "the fw 0x%x is loaded (size=%d)\n",
		 data->fw_img->first_fw_ver, data->fw_img->fw_len);

out:
	kfree(fw_name);
	return ret;
}

static int load_fw_in_sdcard(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;
	mm_segment_t old_fs;
	struct file *fp;
	long nread;
	int len;

	char *fw_name = kasprintf(GFP_KERNEL, "%s%s.in.fw",
				  TC300K_FW_IN_SDCARD_PATH, TC300K_FW_NAME);

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fp = filp_open(fw_name, O_RDONLY, S_IRUSR);
	if (!fp) {
		dev_err(&client->dev, "%s: fail to open fw in %s\n",
			__func__, fw_name);
		ret = -ENOENT;
		goto err_open_fw;
	}
	len = fp->f_path.dentry->d_inode->i_size;

	data->fw_img = kzalloc(len, GFP_KERNEL);
	if (!data->fw_img) {
		dev_err(&client->dev, "%s: fail to alloc mem for fw\n",
			__func__);
		ret = -ENOMEM;
		goto err_alloc;
	}
	nread = vfs_read(fp, (char __user *)data->fw_img, len, &fp->f_pos);

	dev_info(&client->dev, "%s: load fw in internal sd (%ld)\n",
		 __func__, nread);

	ret = 0;

err_alloc:
	filp_close(fp, NULL);
err_open_fw:
	set_fs(old_fs);
	kfree(fw_name);
	return ret;
}

static int tc300k_load_fw(struct tc300k_data *data, u8 fw_path)
{
	struct i2c_client *client = data->client;
	int ret;

	switch (fw_path) {
	case FW_BUILT_IN:
		ret = load_fw_built_in(data);
		break;

	case FW_IN_SDCARD:
		ret = load_fw_in_sdcard(data);
		break;

	default:
		dev_err(&client->dev, "%s: invalid fw path (%d)\n",
			__func__, fw_path);
		return -ENOENT;
	}
	if (ret < 0) {
		dev_err(&client->dev, "fail to load fw in %d (%d)\n",
			fw_path, ret);
		return ret;
	}
	return 0;
}

static int tc300k_unload_fw(struct tc300k_data *data, u8 fw_path)
{
	struct i2c_client *client = data->client;

	switch (fw_path) {
	case FW_BUILT_IN:
		release_firmware(data->fw);
		break;

	case FW_IN_SDCARD:
		kfree(data->fw_img);
		break;

	default:
		dev_err(&client->dev, "%s: invalid fw path (%d)\n",
			__func__, fw_path);
		return -ENOENT;
	}

	return 0;
}

static int tc300k_flash_fw(struct tc300k_data *data, u8 fw_path, bool force)
{
	struct i2c_client *client = data->client;
	int ret;
	int fw_ver;
	int module_ver;

	
	ret = tc300k_load_fw(data, fw_path);
	if (ret < 0) {
		dev_err(&client->dev, "fail to load fw (%d)\n", ret);
		return ret;
	}
	data->cur_fw_path = fw_path;
	/* firmware version compare */	
	fw_ver = tc300k_get_fw_ver(data);

	module_ver = tc300k_get_module_ver(data);
	dev_info(&client->dev, "fw_ver(%#x), module_ver(%#x)\n", fw_ver,module_ver);
	
	if ((fw_ver >= data->fw_img->first_fw_ver) && !force ) {
	ret = HAVE_LATEST_FW;
		data->fdata->fw_update_skip = 1;
		dev_info(&client->dev, "IC aleady have latest firmware (%#x)\n",
			 fw_ver);
		goto out;
	}
	
	dev_info(&client->dev, "fw update to %#x (from %#x) (%s)\n",
		 data->fw_img->first_fw_ver, fw_ver,
		 (force) ? "force" : "ver mismatch");
	data->fdata->fw_update_skip = 0;
	queue_work(data->fw_wq, &data->fw_work);

	return FW_UPDATE_RUNNING;

 out:
	tc300k_unload_fw(data, fw_path);
	return ret;
}

static int tc300k_initialize(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;

	ret = tc300k_flash_fw(data, FW_BUILT_IN, false);

	if (ret < 0)
		dev_err(&client->dev, "fail to flash fw (%d)\n", ret);
	else
		dev_err(&client->dev, "success to flash fw (%d)\n", ret);

	return ret;
}

static void tc300k_led_worker(struct work_struct *work)
{
	struct tc300k_data *data = container_of(work, struct tc300k_data,
					       led_work);
	struct i2c_client *client = data->client;
	u8 buf;
	int ret;
	u8 br;
	int cnt = 20;

	br = data->led_brightness;
	data->led_wq_passed = 1;

#if defined(LED_DEBUG)
	dev_info(&client->dev, "%s: turn %s LED\n", __func__,
		(br == LED_OFF) ? "off" : "on");
#endif

	if (br == LED_OFF)
		buf = TC300K_CMD_LED_OFF;
	else /* LED_FULL*/
		buf = TC300K_CMD_LED_ON;

	if (br == LED_OFF && !data->enabled) {
		dev_info(&client->dev, "%s: ignore LED control "
			 "(IC is disabled)\n", __func__);
		data->led_on = true;
		return;
	}

	while (!data->enabled) {
		data->led_wq_passed = 1;
		msleep(TC300K_POWERON_DELAY);
#if defined(LED_DEBUG)
		dev_err(&client->dev, "%s: waiting for device (%d)\n",
			__func__, cnt);
#endif
		if (--cnt <  0) {
			dev_err(&client->dev, "%s: fail to tc300k led %s\n",
				__func__,
				(br == LED_OFF) ? "OFF" : "ON");
			return;
		}
	}

	ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, buf);
	if (ret < 0)
		dev_err(&client->dev, "%s: failed to wt led data (%d)\n",
			__func__, ret);
}

static void tc300k_led_set(struct led_classdev *led_cdev,
			  enum led_brightness value)
{
	struct tc300k_data *data =
			container_of(led_cdev, struct tc300k_data, led);
	struct i2c_client *client = data->client;

	if (unlikely(wake_lock_active(&data->fw_wake_lock))) {
		dev_info(&client->dev, "fw is being updated."
			 "led control is ignored.\n");
		return;
	}

	if (data->led_brightness == value) {
		dev_info(&client->dev, "%s: ignore LED control "
			 "(set same brightness)\n", __func__);
		return;
	}

	data->led_brightness = value;
	if (data->led_wq)
		queue_work(data->led_wq, &data->led_work);
}

static ssize_t tc300k_fw_ver_ic_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ver;
	int ret;

 	if(data->enabled){
		data->fdata->fw_update_skip = 0;
		ver = tc300k_get_fw_ver(data); 
 	}
	
	ver = data->ic_fw_ver;
	if (ver < 0) {
		dev_err(&client->dev, "%s: fail to read fw ver (%d)\n.",
			__func__, ver);
		ret = sprintf(buf, "%s\n", "error");
		goto out;
	}

	dev_info(&client->dev, "%s: %#x\n", __func__, (u8)ver);
	ret = sprintf(buf, "%#x\n", (u8)ver);
out:
	return ret;
}

static ssize_t tc300k_fw_ver_src_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u16 ver;

	if(data->enabled){
		ret = tc300k_load_fw(data, FW_BUILT_IN);
		if (ret < 0) {
			dev_err(&client->dev, "%s: fail to load fw (%d)\n.", __func__,
				ret);
			ver = 0;
			goto out;
		}
	}

	ver = data->fw_img->first_fw_ver;

	if(data->enabled){
		ret = tc300k_unload_fw(data, FW_BUILT_IN);
		if (ret < 0) {
			dev_err(&client->dev, "%s: fail to unload fw (%d)\n.",
				__func__, ret);
			ver = 0;
			goto out;
		}
	}

out:
	ret = sprintf(buf, "%#x\n", ver);
	dev_info(&client->dev, "%s: %#x\n", __func__, ver);
	return ret;
}

static ssize_t tc300k_fw_update_store(struct device *dev,
				   struct device_attribute *devattr,
				   const char *buf, size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 fw_path;

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled (fw_update_skip =% d)\n.",
			__func__, data->fdata->fw_update_skip);
		return -EPERM;
	}

	switch (*buf) {
	case 's':
	case 'S':
		fw_path = FW_BUILT_IN;
		break;

	case 'i':
	case 'I':
		fw_path = FW_IN_SDCARD;
		break;

	default:
		dev_err(&client->dev, "%s: invalid parameter %c\n.", __func__,
			*buf);
		return -EINVAL;
	}

	data->fdata->fw_flash_status = DOWNLOADING;
	data->enabled = false;

	ret = tc300k_flash_fw(data, fw_path, true);
	if (ret < 0) {
		data->fdata->fw_flash_status = FAIL;
		dev_err(&client->dev, "%s: fail to flash fw (%d)\n.", __func__,
			ret);
		return ret;
	}

	data->fdata->fw_flash_status = PASS;

	return 0;
}

static ssize_t tc300k_fw_update_status_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;

	switch (data->fdata->fw_flash_status) {
	case DOWNLOADING:
		ret = sprintf(buf, "%s\n", TO_STRING(DOWNLOADING));
		break;
	case FAIL:
		ret = sprintf(buf, "%s\n", TO_STRING(FAIL));
		break;
	case PASS:
		ret = sprintf(buf, "%s\n", TO_STRING(PASS));
		break;
	default:
		dev_err(&client->dev, "%s: invalid status\n", __func__);
		ret = 0;
		goto out;
	}

	dev_info(&client->dev, "%s: %#x\n", __func__,
		 data->fdata->fw_flash_status);
	data->fdata->fw_update_skip = 0;

out:
	return ret;
}

static int tc300k_factory_mode_enable(struct i2c_client *client, u8 cmd)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, cmd);
	msleep(TC300K_CMD_DELAY);

	return ret;
}

static ssize_t tc300k_factory_mode(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int scan_buffer;
	int ret;
	u8 cmd;

	ret = sscanf(buf, "%d", &scan_buffer);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(scan_buffer == 0 || scan_buffer == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (data->factory_mode == (bool)scan_buffer) {
		dev_info(&client->dev, "%s same command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (scan_buffer == 1) {
		dev_notice(&client->dev, "factory mode\n");
		cmd = TC300K_CMD_FAC_ON;
	} else {
		dev_notice(&client->dev, "normale mode\n");
		cmd = TC300K_CMD_FAC_OFF;
	}

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		data->factory_mode = (bool)scan_buffer;
		return count;
	}

	ret = tc300k_factory_mode_enable(client, cmd);
	if (ret < 0)
		dev_err(&client->dev, "%s fail(%d)\n", __func__, ret);

	data->factory_mode = (bool)scan_buffer;

	return count;
}

static ssize_t tc300k_factory_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->factory_mode);
}

static ssize_t recent_key_show(struct device *dev,
				      struct device_attribute *attr,
				      char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6] = {0, };
	u16 value;
	

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_2KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_CH_PCK_H_OFFSET] << 8) |
		buff[TC300K_CH_PCK_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t recent_key_ref_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_6KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_CH_PCK_H_OFFSET] << 8) |
		buff[TC300K_CH_PCK_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}


static ssize_t back_key_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	int value;
	u8 buff[6] = {0, };

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_1KEY_DATA, 6, buff);
		if (ret != 6) {
			dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
			return -1;
	}
	value = (buff[TC300K_CH_PCK_H_OFFSET] << 8) |
		buff[TC300K_CH_PCK_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t back_key_ref_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_5KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_CH_PCK_H_OFFSET] << 8) |
		buff[TC300K_CH_PCK_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t dummy_recent_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_4KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_CH_PCK_H_OFFSET] << 8) |
		buff[TC300K_CH_PCK_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t dummy_back_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_3KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_CH_PCK_H_OFFSET] << 8) |
		buff[TC300K_CH_PCK_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t recent_key_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_2KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_RAW_H_OFFSET] << 8) |
		buff[TC300K_RAW_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t recent_key_raw_ref(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_6KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_RAW_H_OFFSET] << 8) |
		buff[TC300K_RAW_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t back_key_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_1KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_RAW_H_OFFSET] << 8) |
		buff[TC300K_RAW_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t back_key_raw_ref(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}
	
	ret = i2c_smbus_read_i2c_block_data(client, TC300K_5KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_RAW_H_OFFSET] << 8) |
		buff[TC300K_RAW_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t dummy_recent_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_4KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_RAW_H_OFFSET] << 8) |
		buff[TC300K_RAW_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t dummy_back_raw(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 buff[6];
	int value;

	if ((!data->enabled)) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -1;
	}

	ret = i2c_smbus_read_i2c_block_data(client, TC300K_3KEY_DATA, 6, buff);
	if (ret != 6) {
		dev_err(&client->dev, "%s read fail(%d)\n", __func__, ret);
		return -1;
	}
	value = (buff[TC300K_RAW_H_OFFSET] << 8) |
		buff[TC300K_RAW_L_OFFSET];

	return sprintf(buf, "%d\n", value);
}

static ssize_t tc300k_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 threshold_h, threshold_l;

	if (!data->enabled) {
		dev_err(&client->dev, "%s: device is disabled\n.", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_byte_data(client, TC300K_THRES_H);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read threshold_h (%d)\n",
			__func__, ret);
		return ret;
	}
	threshold_h = ret;

	ret = i2c_smbus_read_byte_data(client, TC300K_THRES_L);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read threshold_l (%d)\n",
			__func__, ret);
		return ret;
	}
	threshold_l = ret;

	data->threhold = (threshold_h << 8) | threshold_l;

	return sprintf(buf, "%d\n", data->threhold);
}

static int tc300k_glove_mode_enable(struct i2c_client *client, u8 cmd)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, cmd);
	msleep(TC300K_CMD_DELAY);

	return ret;
}

static ssize_t tc300k_glove_mode(struct device *dev,
	 struct device_attribute *attr, const char *buf, size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int scan_buffer;
	int ret;
	u8 cmd;

	ret = sscanf(buf, "%d", &scan_buffer);
	if (ret != 1) {
		dev_err(&client->dev, "%s: cmd read err\n", __func__);
		return count;
	}

	if (!(scan_buffer == 0 || scan_buffer == 1)) {
		dev_err(&client->dev, "%s: wrong command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (data->glove_mode == (bool)scan_buffer) {
		dev_info(&client->dev, "%s same command(%d)\n",
			__func__, scan_buffer);
		return count;
	}

	if (scan_buffer == 1) {
		dev_notice(&client->dev, "factory mode\n");
		cmd = TC300K_CMD_GLOVE_ON;
	} else {
		dev_notice(&client->dev, "normale mode\n");
		cmd = TC300K_CMD_GLOVE_OFF;
	}

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		data->glove_mode = (bool)scan_buffer;
		return count;
	}
	ret = tc300k_glove_mode_enable(client, cmd);
	if (ret < 0)
		dev_err(&client->dev, "%s fail(%d)\n", __func__, ret);

	data->glove_mode = (bool)scan_buffer;

	return count;
}

static ssize_t tc300k_glove_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", data->glove_mode);
}

static ssize_t tc300k_modecheck_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = data->client;
	int ret;
	u8 mode, glove, factory;

	if (!data->enabled) {
		dev_err(&client->dev, "can't excute %s\n", __func__);
		return -EPERM;
	}

	ret = i2c_smbus_read_byte_data(client, TC300K_MODE);
	if (ret < 0) {
		dev_err(&client->dev, "%s: failed to read threshold_h (%d)\n",
			__func__, ret);
		return ret;
	}
	mode = ret;

	glove = ((mode & 0xf0) >> 4);
	factory = mode & 0x0f;

	return sprintf(buf, "glove:%d, factory:%d\n", glove, factory);
}

#ifdef TSP_BOOSTER
static ssize_t boost_level_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct tc300k_data *data = dev_get_drvdata(dev);
	int val, retval;

	dev_info(&data->client->dev, "%s\n", __func__);
	sscanf(buf, "%d", &val);

	if (val != 1 && val != 2 && val != 0) {
		dev_info(&data->client->dev,
			"%s: wrong cmd %d\n", __func__, val);
		return count;
	}
	data->dvfs_boost_mode = val;
	dev_info(&data->client->dev,
			"%s: dvfs_boost_mode = %d\n",
			__func__, data->dvfs_boost_mode);

	if (data->dvfs_boost_mode == DVFS_STAGE_DUAL) {
		data->dvfs_freq = MIN_TOUCH_LIMIT_SECOND;
		dev_info(&data->client->dev,
			"%s: boost_mode DUAL, dvfs_freq = %d\n",
			__func__, data->dvfs_freq);
	} else if (data->dvfs_boost_mode == DVFS_STAGE_SINGLE) {
		data->dvfs_freq = MIN_TOUCH_LIMIT;
		dev_info(&data->client->dev,
			"%s: boost_mode SINGLE, dvfs_freq = %d\n",
			__func__, data->dvfs_freq);
	} else if (data->dvfs_boost_mode == DVFS_STAGE_NONE) {
		data->dvfs_freq = -1;
		retval = set_freq_limit(DVFS_TOUCH_ID, -1);
		if (retval < 0) {
			dev_err(&data->client->dev,
					"%s: booster stop failed(%d).\n",
					__func__, retval);
			data->dvfs_lock_status = false;
		}
	}
	return count;
}
#endif


static DEVICE_ATTR(touchkey_threshold, S_IRUGO, tc300k_threshold_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_panel, S_IRUGO | S_IWUSR | S_IWGRP,
		   tc300k_fw_ver_ic_show, NULL);
static DEVICE_ATTR(touchkey_firm_version_phone, S_IRUGO | S_IWUSR | S_IWGRP,
		   tc300k_fw_ver_src_show, NULL);
static DEVICE_ATTR(touchkey_firm_update, S_IRUGO | S_IWUSR | S_IWGRP,
		   NULL, tc300k_fw_update_store);
static DEVICE_ATTR(touchkey_firm_update_status, S_IRUGO,
		   tc300k_fw_update_status_show, NULL);
static DEVICE_ATTR(touchkey_recent, S_IRUGO, recent_key_show, NULL);
static DEVICE_ATTR(touchkey_recent_ref, S_IRUGO, recent_key_ref_show, NULL);
static DEVICE_ATTR(touchkey_back, S_IRUGO, back_key_show, NULL);
static DEVICE_ATTR(touchkey_back_ref, S_IRUGO, back_key_ref_show, NULL);
static DEVICE_ATTR(touchkey_d_menu, S_IRUGO, dummy_recent_show, NULL);
static DEVICE_ATTR(touchkey_d_back, S_IRUGO, dummy_back_show, NULL);
static DEVICE_ATTR(touchkey_recent_raw, S_IRUGO, recent_key_raw, NULL);
static DEVICE_ATTR(touchkey_recent_raw_ref, S_IRUGO, recent_key_raw_ref, NULL);
static DEVICE_ATTR(touchkey_back_raw, S_IRUGO, back_key_raw, NULL);
static DEVICE_ATTR(touchkey_back_raw_ref, S_IRUGO, back_key_raw_ref, NULL);
static DEVICE_ATTR(touchkey_d_menu_raw, S_IRUGO, dummy_recent_raw, NULL);
static DEVICE_ATTR(touchkey_d_back_raw, S_IRUGO, dummy_back_raw, NULL);
static DEVICE_ATTR(glove_mode, S_IRUGO | S_IWUSR | S_IWGRP,
		tc300k_glove_mode_show, tc300k_glove_mode);
static DEVICE_ATTR(touchkey_factory_mode, S_IRUGO | S_IWUSR | S_IWGRP,
		tc300k_factory_mode_show, tc300k_factory_mode);
static DEVICE_ATTR(modecheck, S_IRUGO, tc300k_modecheck_show, NULL);

#ifdef TSP_BOOSTER
static DEVICE_ATTR(boost_level, S_IWUSR | S_IWGRP, NULL, boost_level_store);
#endif

static struct attribute *touchkey_attributes[] = {
	&dev_attr_touchkey_threshold.attr,
	&dev_attr_touchkey_firm_version_panel.attr,
	&dev_attr_touchkey_firm_version_phone.attr,
	&dev_attr_touchkey_firm_update.attr,
	&dev_attr_touchkey_firm_update_status.attr,
	&dev_attr_touchkey_recent.attr,
	&dev_attr_touchkey_recent_ref.attr,
	&dev_attr_touchkey_back.attr,
	&dev_attr_touchkey_back_ref.attr,
	&dev_attr_touchkey_d_menu.attr,
	&dev_attr_touchkey_d_back.attr,
	&dev_attr_touchkey_recent_raw.attr,
	&dev_attr_touchkey_recent_raw_ref.attr,
	&dev_attr_touchkey_back_raw.attr,
	&dev_attr_touchkey_back_raw_ref.attr,
	&dev_attr_touchkey_d_menu_raw.attr,
	&dev_attr_touchkey_d_back_raw.attr,
	&dev_attr_touchkey_factory_mode.attr,
	&dev_attr_glove_mode.attr,
	&dev_attr_modecheck.attr,
#ifdef TSP_BOOSTER
	&dev_attr_boost_level.attr,
#endif
	NULL,
};

static struct attribute_group touchkey_attr_group = {
	.attrs = touchkey_attributes,
};

static int tc300k_init_interface(struct tc300k_data *data)
{
	struct i2c_client *client = data->client;
	int ret;

	data->fdata->dummy_dev = device_create(sec_class, NULL, (dev_t)NULL,
					       data, TC300K_DEVICE);
	if (IS_ERR(data->fdata->dummy_dev)) {
		dev_err(&client->dev, "Failed to create fac tsp temp dev\n");
		ret = -ENODEV;
		data->fdata->dummy_dev = NULL;
		goto err_create_sec_class_dev;
	}
	
	ret = sysfs_create_group(&data->fdata->dummy_dev->kobj,
				 &touchkey_attr_group);
	if (ret) {
		dev_err(&client->dev, "%s: failed to create fac_attr_group "
			"(%d)\n", __func__, ret);
		ret = (ret > 0) ? -ret : ret;
		goto err_create_fac_attr_group;
	}

	return 0;

	sysfs_remove_group(&data->fdata->dummy_dev->kobj,
			   &touchkey_attr_group);
err_create_fac_attr_group:
	device_destroy(sec_class, (dev_t)NULL);
err_create_sec_class_dev:
	return ret;
}

static void tc300k_destroy_interface(struct tc300k_data *data)
{
	sysfs_remove_group(&data->fdata->dummy_dev->kobj,
			   &touchkey_attr_group);
	device_destroy(sec_class, (dev_t)NULL);
}

static int __devinit tc300k_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{ 	
	struct tc300k_platform_data *pdata;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct tc300k_data *data;
	struct input_dev *input_dev;
	
	int ret;
	int i;
	int err;

	printk(KERN_ERR "%s start\n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C))
		return -EIO;

	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct tc300k_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_info(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		err = tc300k_parse_dt(&client->dev, pdata);
		if (err)
			return err;
	}else
		pdata = client->dev.platform_data;

	data = kzalloc(sizeof(struct tc300k_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev, "failed to alloc memory\n");
		ret = -ENOMEM;
		goto err_data_alloc;
	}
	
	data->pdata = pdata;

	data->fdata = kzalloc(sizeof(struct fdata_struct), GFP_KERNEL);
	if (!data->fdata) {
		dev_err(&client->dev, "failed to alloc memory for fdata\n");
		ret = -ENOMEM;
		goto err_data_alloc_fdata;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&client->dev, "failed to allocate input device\n");
		ret = -ENOMEM;
		goto err_input_devalloc;
	}

	data->client = client;
	data->input_dev = input_dev;
	data->num_key = data->pdata->num_key;
	data->keycodes = data->pdata->keycodes;
	
#if !defined(CONFIG_SAMSUNG_PRODUCT_SHIP)
	for (i = 0; i < data->num_key; i++)
		dev_info(&client->dev, "keycode[%d]= %3d\n", i,
			data->keycodes[i]);
#endif

	pdata->enable = 1;
	tc300k_gpio_request(data);
	tc300k_led_power(data,1);
	tc300k_power(data,1);
	msleep(TC300K_POWERON_DELAY);
	
	data->suspend_type = data->pdata->suspend_type;
	data->scl = data->pdata->gpio_scl;
	data->sda = data->pdata->gpio_sda;
	data->led_wq_passed=0;
	
	mutex_init(&data->lock);
	wake_lock_init(&data->fw_wake_lock, WAKE_LOCK_SUSPEND, "tc300k_fw_wake_lock");

	client->irq=gpio_to_irq(pdata->gpio_int);

	snprintf(data->phys, sizeof(data->phys), "%s/input0", dev_name(&client->dev));
	input_dev->name = "sec_touchkey";
	input_dev->phys = data->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;
	input_dev->keycode = data->keycodes;
	input_dev->keycodesize = sizeof(data->keycodes[0]);
	input_dev->keycodemax = data->num_key;


	set_bit(EV_SYN, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_LED, input_dev->evbit);
	set_bit(LED_MISC, input_dev->ledbit);
	for (i = 0; i < data->num_key; i++) {
		input_set_capability(input_dev, EV_KEY, data->keycodes[i]);
		set_bit(data->keycodes[i], input_dev->keybit);
	}

	i2c_set_clientdata(client, data);
	input_set_drvdata(input_dev, data);
	ret = input_register_device(data->input_dev);
	if (ret) {
		dev_err(&client->dev, "fail to register input_dev (%d).\n",
			ret);
		goto err_register_input_dev;
	}

	ret = tc300k_init_interface(data);
	if (ret < 0) {
		dev_err(&client->dev, "failed to init interface (%d)\n", ret);
		goto err_init_interface;
	}
	
	/* Add symbolic link */
	ret = sysfs_create_link(&data->fdata->dummy_dev->kobj, &input_dev->dev.kobj, "input");
		if (ret < 0) {
			dev_err(&client->dev,
					"%s: Failed to create input symbolic link\n",
					__func__);
		}
	
		dev_set_drvdata(data->fdata->dummy_dev, data);
	

#ifdef USE_OPEN_CLOSE
	input_dev->open = tc300k_input_open;
	input_dev->close = tc300k_input_close;
#endif
	
#ifdef TSP_BOOSTER
		tc300k_init_dvfs(data);
#endif

	data->fw_up_running = false;	
	data->fw_wq = create_singlethread_workqueue(client->name);
	if (!data->fw_wq) {
		dev_err(&client->dev, "fail to create workqueue for fw_wq\n");
		ret = -ENOMEM;
		goto err_create_fw_wq;
	}

	INIT_WORK(&data->fw_work, tc300k_fw_update_worker);	
	
	if (client->irq) {
		ret = request_threaded_irq(client->irq, NULL, tc300k_interrupt,
					IRQF_TRIGGER_FALLING, TC300K_DEVICE, data);
		if (ret) {
			dev_err(&client->dev, "fail to request irq (%d).\n",
				client->irq);
			goto err_request_irq;
		}
	}
	ret =tc300k_initialize(data); //firmware update
	
	if (ret < 0) {
		dev_err(&client->dev, "fail to tc300k initialize (%d).\n",
			ret);
		goto err_initialize;
	}
	
	switch (ret) {
	case HAVE_LATEST_FW:
		data->enabled = true;
		break;
	case FW_UPDATE_RUNNING:
		data->enabled = false;
		break;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = tc300k_early_suspend;
	data->early_suspend.resume = tc300k_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	data->led_wq = create_singlethread_workqueue(client->name);
	if (!data->led_wq) {
		dev_err(&client->dev, "fail to create workqueue for led_wq\n");
		ret = -ENOMEM;
		goto err_create_led_wq;
	}
	INIT_WORK(&data->led_work, tc300k_led_worker);

	data->led.name = "button-backlight";
	data->led.brightness = LED_OFF;
	data->led.max_brightness = LED_FULL;
	data->led.brightness_set = tc300k_led_set;

	ret = led_classdev_register(&client->dev, &data->led);
	if (ret) {
		dev_err(&client->dev, "fail to register led_classdev (%d).\n",
			ret);
		goto err_register_led;
	}

	dev_info(&client->dev, "successfully probed.\n");

	return 0;

err_register_led:
	destroy_workqueue(data->led_wq);
err_create_led_wq:
err_initialize:
	free_irq(client->irq, data);
err_request_irq:
	destroy_workqueue(data->fw_wq);
err_create_fw_wq:
	tc300k_destroy_interface(data);
err_init_interface:
	input_unregister_device(input_dev);
err_register_input_dev:
	wake_lock_destroy(&data->fw_wake_lock);
	input_free_device(input_dev);
err_input_devalloc:
	kfree(data->fdata);
err_data_alloc_fdata:
	kfree(data);
err_data_alloc:
	return ret;
}


static int __devexit tc300k_remove(struct i2c_client *client)
{
	struct tc300k_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	gpio_free(data->pdata->gpio_int);
	led_classdev_unregister(&data->led);
	destroy_workqueue(data->led_wq);
	destroy_workqueue(data->fw_wq);
	input_unregister_device(data->input_dev);
	input_free_device(data->input_dev);
	kfree(data->fdata);
	kfree(data);
	return 0;
}

#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static int tc300k_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc300k_data *data = i2c_get_clientdata(client);
	int i;
	
	mutex_lock(&data->lock);
	/* now the firmware is being updated. IC will keep power state. */
	if (unlikely(wake_lock_active(&data->fw_wake_lock))) {
		dev_info(&data->client->dev, "%s, now fw updating. suspend "
			 "control is ignored.\n", __func__);
		return 0;
	}

	if (!data->enabled) {
		dev_info(&client->dev, "%s, already disabled.\n", __func__);
		return 0;
	}

#ifdef TSP_BOOSTER
	tc300k_set_dvfs_lock(data, 2);
	dev_info(&data->client->dev,
			"%s: dvfs_lock free.\n", __func__);
#endif


	disable_irq(client->irq);
	data->enabled = false;

	/* report not released key */
	for (i = 0; i < data->num_key; i++)
		input_report_key(data->input_dev, data->keycodes[i], 0);
		input_sync(data->input_dev);

	tc300k_led_power(data,false);
	tc300k_power(data,0);		
	data->led_on = false;

	mutex_unlock(&data->lock);
	
	return 0;
}

static int tc300k_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct tc300k_data *data = i2c_get_clientdata(client);
	int ret;
	u8 cmd;

	mutex_lock(&data->lock);

	if (unlikely(wake_lock_active(&data->fw_wake_lock))) {
		dev_info(&data->client->dev, "%s, now fw updating. resume "
			 "control is ignored.\n", __func__);
		goto out;
	}

	if (data->enabled) {
		dev_info(&client->dev, "%s, already enabled.\n", __func__);
		goto out;
	}

	tc300k_led_power(data,true);
	tc300k_power(data,1);
	msleep(200);
	enable_irq(client->irq);
	data->enabled = true;
	if (data->led_on == true) {
		data->led_on = false;
		dev_notice(&client->dev, "led on(resume)\n");
		cmd = TC300K_CMD_LED_ON;
		ret = i2c_smbus_write_byte_data(client, TC300K_CMD_ADDR, cmd);
		if (ret < 0)
			dev_err(&client->dev, "%s led on fail(%d)\n", __func__, ret);
		else
			msleep(TC300K_CMD_DELAY);
	}

	
	if (data->glove_mode) {
		ret = tc300k_glove_mode_enable(client, TC300K_CMD_GLOVE_ON);
		if (ret < 0)
			dev_err(&client->dev, "%s glovemode fail(%d)\n", __func__, ret);
	}
	
	if (data->factory_mode) {
		ret = tc300k_factory_mode_enable(client, TC300K_CMD_FAC_ON);
		if (ret < 0)
			dev_err(&client->dev, "%s factorymode fail(%d)\n", __func__, ret);
	}		
out:
	mutex_unlock(&data->lock);
	dev_info(&client->dev, "%s\n", __func__);
	return 0;
}
#endif


#ifdef USE_OPEN_CLOSE
static int tc300k_input_open(struct input_dev *dev)
{
	struct tc300k_data *data = input_get_drvdata(dev);

	dev_info(&data->client->dev, "%s.\n", __func__);
	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), 1);
	tc300k_resume(&data->client->dev);

	return 0;
}

static void tc300k_input_close(struct input_dev *dev)
{
	struct tc300k_data *data = input_get_drvdata(dev);

	dev_info(&data->client->dev, "%s.\n", __func__);
	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_sda, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	gpio_tlmm_config(GPIO_CFG(data->pdata->gpio_scl, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA), 1);
	tc300k_suspend(&data->client->dev);
}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tc300k_early_suspend(struct early_suspend *h)
{
	struct tc300k_data *data;
	pr_info("tc300k early suspend\n");
	data = container_of(h, struct tc300k_data, early_suspend);
	tc300k_suspend(&data->client->dev);
	pr_info("tc300k early suspend 2\n");
}

static void tc300k_late_resume(struct early_suspend *h)
{
	struct tc300k_data *data;
	pr_info("tc300k late_resume\n");
	data = container_of(h, struct tc300k_data, early_suspend);
	tc300k_resume(&data->client->dev);
	pr_info("tc300k late_resume 2\n");
}
#endif

#if !defined(USE_OPEN_CLOSE)
#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
static const struct dev_pm_ops tc300k_pm_ops = {
	.suspend	= tc300k_suspend,
	.resume 	= tc300k_resume,
};
#endif
#endif

static const struct i2c_device_id tc300k_id[] = {
	{ TC300K_DEVICE, 0 },
	{ }
};

#ifdef CONFIG_OF
static struct of_device_id coreriver_match_table[] = {
	{ .compatible = "coreriver,coreriver-tkey",},    
	{ },
};
#else
#define coreriver_match_table	NULL
#endif

MODULE_DEVICE_TABLE(i2c, tc300k_id);

static struct i2c_driver tc300k_driver = {
	.probe		= tc300k_probe,
	.remove		= tc300k_remove,
	.driver = {
		.name	= TC300K_DEVICE,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND) && !defined(USE_OPEN_CLOSE)
		.pm	= &tc300k_pm_ops,
#endif
#ifdef CONFIG_OF
		.of_match_table = coreriver_match_table,
#endif	
	},
	.id_table	= tc300k_id,
};

static int __init tc300k_init(void)
{
	int ret = 0;

#ifdef CONFIG_SAMSUNG_LPM_MODE
	if (poweroff_charging) {
		pr_notice("%s : LPM Charging Mode!!\n", __func__);
		return 0;
	}
#endif
	
	ret = i2c_add_driver(&tc300k_driver);
	if (ret) {
		printk(KERN_ERR "coreriver touch keypad registration failed. ret= %d\n",
			ret);
	}
	printk(KERN_ERR "%s: init done %d\n", __func__, ret);

	return ret;
}

static void __exit tc300k_exit(void)
{
	i2c_del_driver(&tc300k_driver);
}

module_init(tc300k_init);
module_exit(tc300k_exit);

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("Touchkey driver for Coreriver TC300K");
MODULE_LICENSE("GPL");

