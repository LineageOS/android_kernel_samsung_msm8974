/* drivers/motor/isa1200_vibrator.c
 *
 * Copyright (C) 2011 Samsung Electronics Co. Ltd. All Rights Reserved.
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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/hrtimer.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wakelock.h>
#include <linux/io.h>
#include <linux/vibrator.h>
#include <linux/i2c.h>
#include <linux/earlysuspend.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/krait-regulator.h>

#include "../staging/android/timed_output.h"
#include "isa1200_vibrator.h"


static struct hrtimer timer;

static int max_timeout = 10000;
static int vibrator_value = 0;
static int vibrator_work;
static int isa1200_enabled;


static void _set_vibrator_work(struct work_struct *unused);

static DECLARE_WORK(vib_work, _set_vibrator_work);
struct vibrator_platform_data_isa1200 vibrator_drvdata;

#if defined (CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON) || defined (CONFIG_MACH_T10_3G_OPEN)
static void haptic_power_onoff(int onoff)
{
	int ret;
	static struct regulator *reg_l6;

	if (!reg_l6) {
		reg_l6 = regulator_get(&vibrator_drvdata.client->dev,"vddo");
		if (IS_ERR(reg_l6)) {
			printk(KERN_ERR"could not get 8226_l6, rc = %ld\n",
				PTR_ERR(reg_l6));
			return;
		}
		ret = regulator_set_voltage(reg_l6, 1800000, 1800000);
	}

	if (onoff) {
		ret = regulator_enable(reg_l6);
		if (ret) {
			printk(KERN_ERR"enable l6 failed, rc=%d\n", ret);
			return;
		}
		printk(KERN_DEBUG"haptic power_on is finished.\n");
	} else {
		if (regulator_is_enabled(reg_l6)) {
			ret = regulator_disable(reg_l6);
			if (ret) {
				printk(KERN_ERR"disable l6 failed, rc=%d\n",
									ret);
				return;
			}
		}
		printk(KERN_DEBUG"haptic power_off is finished.\n");
	}
}
#endif

static void vibe_set_pwm_freq(int intensity)
{
	int32_t calc_d;

	/* Put the MND counter in reset mode for programming */
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_SEL_VAL_BMSK,0 << HWIO_GP_SRC_SEL_VAL_SHFT); //SRC_SEL = 000(cxo)
#if defined (CONFIG_MACH_MATISSELTE_ATT)
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_DIV_VAL_BMSK,23 << HWIO_GP_SRC_DIV_VAL_SHFT); //SRC_DIV = 11111 (Div 12)
#else
#if defined (CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON)
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_DIV_VAL_BMSK,29 << HWIO_GP_SRC_DIV_VAL_SHFT); //SRC_DIV = 11111 (Div 16)
#else
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_DIV_VAL_BMSK,31 << HWIO_GP_SRC_DIV_VAL_SHFT); //SRC_DIV = 11111 (Div 16)
#endif
#endif//CONFIG_MACH_MATISSELTE_ATT
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_MODE_VAL_BMSK,2 << HWIO_GP_MODE_VAL_SHFT); //Mode Select 10
	//M value
	HWIO_OUTM(GP_M_REG, HWIO_GP_MD_REG_M_VAL_BMSK,g_nlra_gp_clk_m << HWIO_GP_MD_REG_M_VAL_SHFT);
	if (intensity > 0){
		calc_d = g_nlra_gp_clk_n - (((intensity * g_nlra_gp_clk_pwm_mul) >> 8));
		calc_d = calc_d * motor_strength /100;
		if(calc_d < motor_min_strength)
			calc_d = motor_min_strength;
	}
	else {
		calc_d = ((intensity * g_nlra_gp_clk_pwm_mul) >> 8) + g_nlra_gp_clk_d;
		if(g_nlra_gp_clk_n - calc_d > g_nlra_gp_clk_n * motor_strength /100)
			calc_d = g_nlra_gp_clk_n - g_nlra_gp_clk_n * motor_strength /100;
	}
	// D value
	HWIO_OUTM(GP_D_REG, HWIO_GP_MD_REG_D_VAL_BMSK,(~((int16_t)calc_d << 1)) << HWIO_GP_MD_REG_D_VAL_SHFT);
	//N value
	HWIO_OUTM(GP_NS_REG, HWIO_GP_NS_REG_GP_N_VAL_BMSK,~(g_nlra_gp_clk_n - g_nlra_gp_clk_m) << 0);
}


static void vibe_pwm_onoff(u8 onoff)
{
	if (onoff) {
                HWIO_OUTM(GP1_CMD_RCGR,HWIO_UPDATE_VAL_BMSK,
                                        1 << HWIO_UPDATE_VAL_SHFT);//UPDATE ACTIVE
                HWIO_OUTM(GP1_CMD_RCGR,HWIO_ROOT_EN_VAL_BMSK,
                                        1 << HWIO_ROOT_EN_VAL_SHFT);//ROOT_EN
                HWIO_OUTM(CAMSS_GP1_CBCR, HWIO_CLK_ENABLE_VAL_BMSK,
                                        1 << HWIO_CLK_ENABLE_VAL_SHFT); //CLK_ENABLE
        } else {
		HWIO_OUTM(GP1_CMD_RCGR,HWIO_UPDATE_VAL_BMSK,
                                        0 << HWIO_UPDATE_VAL_SHFT);
                HWIO_OUTM(GP1_CMD_RCGR,HWIO_ROOT_EN_VAL_BMSK,
                                        0 << HWIO_ROOT_EN_VAL_SHFT);
                HWIO_OUTM(CAMSS_GP1_CBCR, HWIO_CLK_ENABLE_VAL_BMSK,
                                        0 << HWIO_CLK_ENABLE_VAL_SHFT);
        }
}

static int vib_isa1200_onoff(u8 onoff)
{

	if(onoff) {
		vibrator_write_register(HCTRL0, 0x88);	//HCTRL0
                vibrator_write_register(HCTRL1, 0x4B);
	} else {
		vibrator_write_register(HCTRL0, 0x08);
	}
	return 0;
}

static void vibe_set_intensity(int intensity)
{
	if (0 == intensity)
		vibe_pwm_onoff(0);
	else {
		if (MAX_INTENSITY == intensity)
			intensity = 1;
		else if (0 != intensity) {
			int tmp = MAX_INTENSITY - intensity;
			intensity = (tmp / 79);	// 79 := 10000 / 127
		}

		vibe_set_pwm_freq(intensity);
		vibe_pwm_onoff(1);
	}
}

static void isa1200_initialize(void)
{
	gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_clk,0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	msleep(1);
	vibrator_write_register(HCTRL0, 0x08);
	vibrator_write_register(HCTRL1, 0x4B);
}

static int set_vibrator(int timeout)
{
	if(timeout) {
#if defined (CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON) || defined (CONFIG_MACH_T10_3G_OPEN)
		vibrator_drvdata.power_onoff(1);
		gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_clk,  3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
#else
		gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_clk,  6, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
#endif
		gpio_set_value(vibrator_drvdata.vib_clk, VIBRATION_ON);
		gpio_set_value(vibrator_drvdata.motor_en, VIBRATION_ON);
#if defined (CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON)
		msleep(1);
#endif
		vib_isa1200_onoff(1);
	} else {
		vib_isa1200_onoff(0);
#if defined (CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON) || defined (CONFIG_MACH_T10_3G_OPEN)
		vibrator_drvdata.power_onoff(0);
#endif
		gpio_set_value(vibrator_drvdata.motor_en, VIBRATION_OFF);
		gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_clk,  0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),GPIO_CFG_ENABLE);
		gpio_set_value(vibrator_drvdata.vib_clk, VIBRATION_OFF);
	}

	vibrator_value = timeout;
	return 0;
}

static void _set_vibrator_work(struct work_struct *unused)
{
	set_vibrator(vibrator_work);
	return;
}


static enum hrtimer_restart vibtation_timer_func(struct hrtimer *timer)
{
	vibrator_work = 0;
	schedule_work(&vib_work);
	return HRTIMER_NORESTART;
}

static int get_time_for_vibtation(struct timed_output_dev *dev)
{
	int remaining;

	if (hrtimer_active(&timer)) {
		ktime_t r = hrtimer_get_remaining(&timer);
		remaining = (int)ktime_to_ms(r);
	} else
		remaining = 0;

	if (vibrator_value == -1)
		remaining = -1;

	return remaining;

}

static void enable_vibtation_from_user(struct timed_output_dev *dev,int value)
{
	printk("[VIB] : time = %d msec \n", value);
	hrtimer_cancel(&timer);
	/* set_vibrator(value); */
	vibrator_work = value;
	schedule_work(&vib_work);

	if (value > 0)
	{
		if (value > max_timeout)
			value = max_timeout;

		hrtimer_start(&timer,ktime_set(value / 1000, (value % 1000) * 1000000),
						HRTIMER_MODE_REL);
		vibrator_value = 0;
	}
}

static struct timed_output_dev timed_output_vt = {
	.name     = "vibrator",
	.get_time = get_time_for_vibtation,
	.enable   = enable_vibtation_from_user,
};

static ssize_t intensity_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	int ret = 0, set_intensity = 0; 

	ret = kstrtoint(buf, 0, &set_intensity);

	if ((set_intensity < 0) || (set_intensity > MAX_INTENSITY)) {
		pr_err("[VIB]: %sout of rage\n", __func__);
		return -EINVAL;
	}

	vibe_set_intensity(set_intensity);
	vibrator_drvdata.intensity = set_intensity;

	return count;
}

static ssize_t intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "intensity: %u\n", vibrator_drvdata.intensity);
}
static DEVICE_ATTR(intensity, 0660, intensity_show, intensity_store);

static void vibtation_start(void)
{
	int ret = 0;

	hrtimer_init(&timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer.function = vibtation_timer_func;

	ret = timed_output_dev_register(&timed_output_vt);
	if(ret)
		pr_info("[VIB]: timed_output_dev_register is fail\n");

	ret = sysfs_create_file(&timed_output_vt.dev->kobj, &dev_attr_intensity.attr);
	if (ret < 0) {
		pr_err("[VIB]: Failed to register sysfs intensity: %d\n", ret);
	}

}

/* Module info */

int vibrator_write_register(u8 addr, u8 w_data)
{

	if (i2c_smbus_write_byte_data(vibrator_drvdata.client, addr, w_data) < 0) {
		pr_err("%s: Failed to write addr=[0x%x], data=[0x%x]\n",
		   __func__, addr, w_data);
		return -1;
	}

	return 0;
}


static int isa1200_parse_dt(struct device *dev)
{
	struct device_node *np = dev->of_node;
	vibrator_drvdata.motor_en = of_get_named_gpio(np, "isa1200,motor_en",0);
	vibrator_drvdata.vib_clk = of_get_named_gpio(np, "isa1200,vib_clk",0);
#if defined(CONFIG_MACH_T10_3G_OPEN)
	gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.motor_en,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),GPIO_CFG_DISABLE);
#endif
	return 0;
}


static int __devinit isa1200_vibrator_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{

	int error;   /* initialized below */
	pr_info("[VIB]: %s\n", __func__);
	motor_min_strength = g_nlra_gp_clk_n*MOTOR_MIN_STRENGTH/100;

	vibrator_drvdata.client = client;
	if(!(client->dev.of_node)){
		pr_info("[VIB]: %s failed, DT is NULL", __func__);
		return -ENODEV;
	}
	error = isa1200_parse_dt(&client->dev);
	if (error)
		return error;

	if( gpio_request(vibrator_drvdata.motor_en, "MOTOR_EN") < 0)
	{
		return -EINVAL;
	}

	gpio_direction_output(vibrator_drvdata.motor_en, 0);
	gpio_export(vibrator_drvdata.motor_en, 0);

	virt_mmss_gp1_base = ioremap(MSM_MMSS_GP1_BASE,0x28);
	if (!virt_mmss_gp1_base)
		panic("[VIB]: Unable to ioremap MSM_MMSS_GP1 memory!");

#if defined(CONFIG_MACH_MATISSE3G_OPEN) || defined (CONFIG_SEC_MATISSELTE_COMMON) || defined (CONFIG_MACH_T10_3G_OPEN)
	vibrator_drvdata.power_onoff = haptic_power_onoff;
#endif
	vibrator_drvdata.intensity = MAX_INTENSITY;
	isa1200_initialize();
	vibe_set_intensity(vibrator_drvdata.intensity);

	isa1200_enabled = 1;

	vibtation_start();

	return 0;

}

static int __devexit isa1200_remove(struct i2c_client *client)
{
	pr_info("[VIB]: isa1200_remove_module.\n");
	iounmap(virt_mmss_gp1_base);
	gpio_free(vibrator_drvdata.motor_en);
	return 0;
}

static int isa1200_vibrator_suspend(struct i2c_client *client,pm_message_t mesg)
{

	int ret = 0;

	if(isa1200_enabled){
		vibrator_write_register(0x30, 0x08);
		gpio_set_value(vibrator_drvdata.motor_en, VIBRATION_OFF);
		isa1200_enabled = 0;
	}
	return ret;

}

static int isa1200_vibrator_resume(struct i2c_client *client)
{
	pr_info("[VIB]:isa1200_vibrator_resume\n");
	return 0;
}

static const struct i2c_device_id isa1200_vibrator_device_id[] = {
	{"isa1200_vibrator", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, isa1200_vibrator_device_id);

static struct of_device_id isa1200_vibrator_table[] = {
	{ .compatible = "isa1200_vibrator,vibrator",},
	{ },
};

static struct i2c_driver isa1200_vibrator_i2c_driver = {
	.driver = {
		.name = "isa1200_vibrator",
		.owner = THIS_MODULE,
		.of_match_table = isa1200_vibrator_table,
	},
	.probe		= isa1200_vibrator_i2c_probe,
	.id_table	= isa1200_vibrator_device_id,
	.suspend	= isa1200_vibrator_suspend,
	.resume		= isa1200_vibrator_resume,
	.remove		= __devexit_p(isa1200_remove),
};

static int __init isa1200_vibrator_init(void)
{
	return(i2c_add_driver(&isa1200_vibrator_i2c_driver));
}

static void __exit isa1200_vibrator_exit(void)
{
	i2c_del_driver(&isa1200_vibrator_i2c_driver);
}

module_init(isa1200_vibrator_init);
module_exit(isa1200_vibrator_exit);

MODULE_AUTHOR("Samsung Corporation");
MODULE_DESCRIPTION("Vibrator driver for the isa1200 ic");
MODULE_LICENSE("GPL v2");
