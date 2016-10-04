
/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/vibrator.h>
#include <mach/msm_iomap.h>
#include <linux/mfd/pm8xxx/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "../staging/android/timed_output.h"

#include "ss_vibrator.h"

/* default timeout */
#define VIB_DEFAULT_TIMEOUT 10000

struct vibrator_platform_data vibrator_drvdata;

struct ss_vib {
	struct device *dev;
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	struct work_struct work;
	struct workqueue_struct *queue;
	struct mutex lock;

	int state;
	int timeout;
	int intensity;
	int timevalue;
};

void vibe_set_intensity(int intensity)
{
	if (intensity == 0)
		vibe_pwm_onoff(0);
	else {
		if (intensity == MAX_INTENSITY)
			intensity = 1;
		else if (intensity != 0) {
			int tmp = MAX_INTENSITY - intensity;
			intensity = (tmp / 79);	// 79 := 10000 / 127
		}
			
		if (vibrator_drvdata.is_pmic_vib_pwm){ 
			//PMIC  PWM
			if (vib_config_pwm_device() < 0)
				pr_err("%s vib_config_pwm_device failed\n", __func__);
		} else {
			vibe_set_pwm_freq(intensity);
			vibe_pwm_onoff(1);
		}
	}
}

int32_t vibe_set_pwm_freq(int intensity)
{
	int32_t calc_d;

	/* Put the MND counter in reset mode for programming */
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_SEL_VAL_BMSK, 
				0 << HWIO_GP_SRC_SEL_VAL_SHFT); //SRC_SEL = 000(cxo)
#if defined(CONFIG_SEC_BERLUTI_PROJECT) || defined(CONFIG_MACH_S3VE3G_EUR)
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_DIV_VAL_BMSK,
				23 << HWIO_GP_SRC_DIV_VAL_SHFT); //SRC_DIV = 10111 (Div 12)
#else
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_SRC_DIV_VAL_BMSK,
				31 << HWIO_GP_SRC_DIV_VAL_SHFT); //SRC_DIV = 11111 (Div 16)
#endif
	HWIO_OUTM(GP1_CFG_RCGR, HWIO_GP_MODE_VAL_BMSK, 
				2 << HWIO_GP_MODE_VAL_SHFT); //Mode Select 10
	//M value
	HWIO_OUTM(GP_M_REG, HWIO_GP_MD_REG_M_VAL_BMSK,
		g_nlra_gp_clk_m << HWIO_GP_MD_REG_M_VAL_SHFT);

#if defined(CONFIG_MACH_LT03EUR) || defined(CONFIG_MACH_LT03SKT)\
	|| defined(CONFIG_MACH_LT03KTT)	|| defined(CONFIG_MACH_LT03LGT) || defined(CONFIG_MACH_PICASSO_LTE)

	if (intensity > 0){
		calc_d = g_nlra_gp_clk_n - (((intensity * g_nlra_gp_clk_pwm_mul) >> 8));
		if(calc_d < motor_min_strength)
			calc_d = motor_min_strength;
		else
			calc_d = (calc_d - motor_min_strength) \
				* (g_nlra_gp_clk_n * motor_strength / 100 - motor_min_strength) \
				/ (g_nlra_gp_clk_n - motor_min_strength) + motor_min_strength;
	}
	else{
		calc_d = ((intensity * g_nlra_gp_clk_pwm_mul) >> 8) + g_nlra_gp_clk_d;
		if(g_nlra_gp_clk_n - calc_d > g_nlra_gp_clk_n * motor_strength /100)
			calc_d = g_nlra_gp_clk_n - g_nlra_gp_clk_n * motor_strength /100;
	}
#else
	if (intensity > 0){
		calc_d = g_nlra_gp_clk_n - (((intensity * g_nlra_gp_clk_pwm_mul) >> 8));
		calc_d = calc_d * motor_strength /100;
		if(calc_d < motor_min_strength)
			calc_d = motor_min_strength;
	}
	else{
		calc_d = ((intensity * g_nlra_gp_clk_pwm_mul) >> 8) + g_nlra_gp_clk_d;
		if(g_nlra_gp_clk_n - calc_d > g_nlra_gp_clk_n * motor_strength /100)
			calc_d = g_nlra_gp_clk_n - g_nlra_gp_clk_n * motor_strength /100;
	}
#endif
	// D value
	HWIO_OUTM(GP_D_REG, HWIO_GP_MD_REG_D_VAL_BMSK,
	 (~((int16_t)calc_d << 1)) << HWIO_GP_MD_REG_D_VAL_SHFT);
	
	//N value	
	HWIO_OUTM(GP_NS_REG, HWIO_GP_NS_REG_GP_N_VAL_BMSK,
	 ~(g_nlra_gp_clk_n - g_nlra_gp_clk_m) << 0);
	
	return VIBRATION_SUCCESS;
}

int32_t vibe_pwm_onoff(u8 onoff)
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
	return VIBRATION_SUCCESS;
}

int vib_config_pwm_device(void)
{
	int ret = 0;
	if(vibrator_drvdata.pwm_dev == NULL){
	//u32	pwm_period_us, duty_us;
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
	vibrator_drvdata.pwm_dev = pwm_request(0,"lpg_3"); // 0 index for LPG3 channel. 
#else
	vibrator_drvdata.pwm_dev = pwm_request(0,"lpg_1"); // 0 index for LPG1 channel. 
#endif

	if (IS_ERR_OR_NULL(vibrator_drvdata.pwm_dev)) {
		pr_err("could not acquire PWM Channel 0, "
						"error %ld\n",PTR_ERR(vibrator_drvdata.pwm_dev));
		vibrator_drvdata.pwm_dev = NULL;
		return -ENODEV;
	}
	//pwm_period_us = 19; // 2000000; 
	//duty_us = 18; //1000000; (90% Duty Cycle)
	
	ret = pwm_config(vibrator_drvdata.pwm_dev,
					 vibrator_drvdata.duty_us,
					 vibrator_drvdata.pwm_period_us); 
	if (ret) {
		pr_err("pwm_config in vibrator enable failed %d\n", ret);
		return ret;
	}
	ret = pwm_enable(vibrator_drvdata.pwm_dev);	
	if (ret < 0) {
		pr_err("pwm_enable in vibrator  failed %d\n", ret);
		return ret;
	}
	} else {
		ret = pwm_enable(vibrator_drvdata.pwm_dev); 
		if (ret < 0) {
			pr_err("pwm_enable in vibrator  failed %d\n", ret);
			return ret;
		}
	}
	return ret;
}

static void set_vibrator(struct ss_vib *vib)
{
	pr_info("[VIB]: %s, value[%d]\n", __func__, vib->state);
	if (vib->state) {
		if(vibrator_drvdata.is_pmic_vib_pwm){ //PMIC PWM
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_ON);
		} else {	//AP PWM
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
			defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_JS01LTESBM)
			gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,\
						2, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_ON);
#elif defined(CONFIG_SEC_BERLUTI_PROJECT) || defined(CONFIG_MACH_S3VE3G_EUR) || defined(CONFIG_MACH_VICTOR3GDSDTV_LTN) || defined(CONFIG_SEC_HESTIA_PROJECT)
			gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,\
						3, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_ON);
#else
			gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,\
						6, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, \
						GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_ON);
#endif
		}
		printk(KERN_DEBUG "[VIB] : %s\n", __func__);
		if (vibrator_drvdata.power_onoff) {
			if (!vibrator_drvdata.changed_chip)
				vibrator_drvdata.power_onoff(1);
		}

#if defined(CONFIG_MOTOR_DRV_MAX77804K)
		if (vibrator_drvdata.changed_chip) {
			gpio_direction_output(vibrator_drvdata.changed_en_gpio, VIBRATION_ON);
			gpio_set_value(vibrator_drvdata.changed_en_gpio,VIBRATION_ON);
		}
#elif defined(CONFIG_MOTOR_DRV_MAX77888)
		max77888_gpio_en(1);
#elif defined(CONFIG_MOTOR_DRV_DRV2603)
		drv2603_gpio_en(1);
#elif defined(CONFIG_MOTOR_ISA1000)
		gpio_direction_output(vibrator_drvdata.vib_en_gpio,VIBRATION_ON);
		gpio_set_value(vibrator_drvdata.vib_en_gpio,VIBRATION_ON);
#endif
		hrtimer_start(&vib->vib_timer, ktime_set(vib->timevalue / 1000,
			(vib->timevalue % 1000) * 1000000),HRTIMER_MODE_REL);
	} else {
		if(vibrator_drvdata.is_pmic_vib_pwm){  //PMIC PWM 
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_OFF);
			if(vibrator_drvdata.pwm_dev != NULL) //Disable the PWM device.
				pwm_disable(vibrator_drvdata.pwm_dev);
		} else{	//AP PWM
#if defined(CONFIG_MACH_S3VE3G_EUR) || defined(CONFIG_MACH_VICTOR3GDSDTV_LTN)
			gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,\
						0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, \
						GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_OFF);
#else
			gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,\
						0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, \
						GPIO_CFG_2MA),GPIO_CFG_ENABLE);
			gpio_set_value(vibrator_drvdata.vib_pwm_gpio, \
					VIBRATION_OFF);
#endif
		}
		printk(KERN_DEBUG "[VIB] : %s\n", __func__);
		if (vibrator_drvdata.power_onoff) {
			if (!vibrator_drvdata.changed_chip)
				vibrator_drvdata.power_onoff(0);
		}
#if defined(CONFIG_MOTOR_DRV_MAX77804K)
		if (vibrator_drvdata.changed_chip) {
			gpio_direction_output(vibrator_drvdata.changed_en_gpio, VIBRATION_OFF);
			gpio_set_value(vibrator_drvdata.changed_en_gpio,VIBRATION_OFF);
		}
#elif defined(CONFIG_MOTOR_DRV_MAX77888)
		max77888_gpio_en(0);
#elif defined(CONFIG_MOTOR_DRV_DRV2603)
		drv2603_gpio_en(0);
#elif defined(CONFIG_MOTOR_ISA1000)
		gpio_direction_output(vibrator_drvdata.vib_en_gpio,VIBRATION_OFF);
		gpio_set_value(vibrator_drvdata.vib_en_gpio,VIBRATION_OFF);
#endif
	}
}

static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	struct ss_vib *vib = container_of(dev, struct ss_vib, timed_dev);

	mutex_lock(&vib->lock);
	hrtimer_cancel(&vib->vib_timer);

	if (value == 0) {
		pr_info("[VIB]: OFF\n");
		vib->state = 0;
	} else {
		pr_info("[VIB]: ON, Duration : %d msec, intensity : %d\n",
			value, vib->intensity);
		vib->state = 1;
		vib->timevalue = value;
	}
	mutex_unlock(&vib->lock);
	queue_work(vib->queue, &vib->work);
}

static void ss_vibrator_update(struct work_struct *work)
{
	struct ss_vib *vib = container_of(work, struct ss_vib, work);

	set_vibrator(vib);
}

static int vibrator_get_time(struct timed_output_dev *dev)
{
	struct ss_vib *vib = container_of(dev, struct ss_vib, timed_dev);

	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);
		return (int)ktime_to_us(r);
	} else
		return 0;
}

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
	struct ss_vib *vib = container_of(timer, struct ss_vib, vib_timer);

	vib->state = 0;
	queue_work(vib->queue, &vib->work);

	return HRTIMER_NORESTART;
}

#ifdef CONFIG_PM
static int ss_vibrator_suspend(struct device *dev)
{
	struct ss_vib *vib = dev_get_drvdata(dev);

	pr_info("[VIB]: %s\n", __func__);

	hrtimer_cancel(&vib->vib_timer);
	cancel_work_sync(&vib->work);
	/* turn-off vibrator */
	vib->state = 0;
	set_vibrator(vib);

#if defined(CONFIG_MOTOR_DRV_MAX77803)
	max77803_vibtonz_en(0);
#elif defined(CONFIG_MOTOR_DRV_MAX77804K)
	if (!vibrator_drvdata.changed_chip)
		max77804k_vibtonz_en(0);
#elif defined(CONFIG_MOTOR_DRV_MAX77828)
	max77828_vibtonz_en(0);
#elif defined(CONFIG_MOTOR_DRV_MAX77888)
	max77888_vibtonz_en(0);
#endif

	return 0;
}

static int ss_vibrator_resume(struct device *dev)
{
	struct ss_vib *vib = dev_get_drvdata(dev);

	pr_info("[VIB]: %s, intensity : %d\n", __func__, vib->intensity);

#if defined(CONFIG_MOTOR_DRV_MAX77803)
	max77803_vibtonz_en(1);
#elif defined(CONFIG_MOTOR_DRV_MAX77804K)
	if (!vibrator_drvdata.changed_chip)
		max77804k_vibtonz_en(1);
#elif defined(CONFIG_MOTOR_DRV_MAX77828)
	max77828_vibtonz_en(1);
#elif defined(CONFIG_MOTOR_DRV_MAX77888)
	max77888_vibtonz_en(1);
#endif

	return 0;
}

#endif

static SIMPLE_DEV_PM_OPS(vibrator_pm_ops, ss_vibrator_suspend, ss_vibrator_resume);

static int vibrator_parse_dt(struct ss_vib *vib)
{
	struct device_node *np = vib->dev->of_node;
	int rc;

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || defined (CONFIG_MACH_JS01LTEDCM)
	vibrator_drvdata.vib_pwm_gpio = of_get_named_gpio(np, "samsung,pmic_vib_pwm_jpn", 0);
#else
	vibrator_drvdata.vib_pwm_gpio = of_get_named_gpio(np, "samsung,pmic_vib_pwm", 0);
#endif

	if (!gpio_is_valid(vibrator_drvdata.vib_pwm_gpio)) {
		pr_err("%s:%d, reset gpio not specified\n",
				__func__, __LINE__);
	}

#if defined(CONFIG_MOTOR_ISA1000)
	vibrator_drvdata.vib_en_gpio = of_get_named_gpio(np, "samsung,vib_en_gpio", 0);
#endif

#if defined(CONFIG_MOTOR_DRV_DRV2603)
	vibrator_drvdata.drv2603_en_gpio = of_get_named_gpio(np, "samsung,drv2603_en", 0);
	if (!gpio_is_valid(vibrator_drvdata.drv2603_en_gpio)) {
		pr_err("%s:%d, drv2603_en_gpio not specified\n",
				__func__, __LINE__);
	}
#endif
#if defined(CONFIG_MOTOR_DRV_MAX77888)
	vibrator_drvdata.max77888_en_gpio = of_get_named_gpio(np, "samsung,vib_power_en", 0);
	if (!gpio_is_valid(vibrator_drvdata.max77888_en_gpio)) {
		pr_err("%s:%d, max77888_en_gpio not specified\n",__func__, __LINE__);
	}
#endif
	
	rc = of_property_read_u32(np, "samsung,pmic_vib_en", &vibrator_drvdata.is_pmic_vib_en);
	if (rc) {
		pr_err("%s:%d, is_pmic_vib_en not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	
	rc = of_property_read_u32(np, "samsung,pmic_haptic_pwr_en", &vibrator_drvdata.is_pmic_haptic_pwr_en);
	if (rc) {
		pr_err("%s:%d, is_pmic_haptic_pwr_en not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	
	//vibrator_drvdata.is_pmic_vib_pwm = 0;  AP PWM PIN
	//vibrator_drvdata.is_pmic_vib_pwm = 1;  PMIC PWM PIN
	rc = of_property_read_u32(np, "samsung,is_pmic_vib_pwm", &vibrator_drvdata.is_pmic_vib_pwm);
	if (rc) {
		pr_err("%s:%d, is_pmic_vib_pwm not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}

	rc = of_property_read_u32(np, "samsung,pwm_period_us", &vibrator_drvdata.pwm_period_us);
	if (rc) {
		pr_err("%s:%d, pwm_period_us not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	rc = of_property_read_u32(np, "samsung,duty_us", &vibrator_drvdata.duty_us);
	if (rc) {
		pr_err("%s:%d, duty_us not specified\n",
						__func__, __LINE__);
		return -EINVAL;
	}
	rc = of_property_read_u32(np, "samsung,changed_chip", &vibrator_drvdata.changed_chip);
	if (rc) {
		pr_info("%s:%d, changed_chip not specified\n",	__func__, __LINE__);
		vibrator_drvdata.changed_chip = 0;
		rc = 0;
	} else {
		if (vibrator_drvdata.changed_chip)
			vibrator_drvdata.changed_en_gpio = of_get_named_gpio(np, "samsung,changed_en_gpio", 0);

	}
	return rc;
}

#if defined(CONFIG_MOTOR_DRV_MAX77804K) || defined(CONFIG_MOTOR_DRV_MAX77828)
static void max77803_haptic_power_onoff(int onoff)
{
	int ret;
	static struct regulator *reg_l23;

	if (!reg_l23) {
		reg_l23 = regulator_get(NULL, "8084_l23");
		ret = regulator_set_voltage(reg_l23, 3000000, 3000000);
		if (IS_ERR(reg_l23)) {
			printk(KERN_ERR"could not get 8084_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return;
		}
	}

	if (onoff) {
		ret = regulator_enable(reg_l23);
		if (ret) {
			printk(KERN_ERR"enable l23 failed, rc=%d\n", ret);
			return;
		}
		printk(KERN_DEBUG"haptic power_on is finished.\n");
	} else {
		if (regulator_is_enabled(reg_l23)) {
			ret = regulator_disable(reg_l23);
			if (ret) {
				printk(KERN_ERR"disable l23 failed, rc=%d\n",
									ret);
				return;
			}
		}
		printk(KERN_DEBUG"haptic power_off is finished.\n");
	}
}
#endif


#if defined(CONFIG_MOTOR_DRV_MAX77803)
static void max77803_haptic_power_onoff(int onoff)
{
	int ret;
#if defined(CONFIG_SEC_H_PROJECT) || defined(CONFIG_SEC_MONTBLANC_PROJECT) || defined(CONFIG_SEC_JS_PROJECT) || \
    defined(CONFIG_MACH_FLTEEUR) || defined(CONFIG_MACH_FLTESKT) || defined(CONFIG_MACH_JVELTEEUR) ||\
    defined(CONFIG_MACH_VIKALCU) || defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
	static struct regulator *reg_l23;

	if (!reg_l23) {
		reg_l23 = regulator_get(NULL, "8941_l23");
#if defined(CONFIG_MACH_FLTESKT)
		ret = regulator_set_voltage(reg_l23, 3000000, 3000000);
#elif defined(CONFIG_MACH_HLTEVZW)
		ret = regulator_set_voltage(reg_l23, 3100000, 3100000);
#elif defined(CONFIG_SEC_LOCALE_KOR_FRESCO)
		ret = regulator_set_voltage(reg_l23, 2488000,2488000);
#else
		ret = regulator_set_voltage(reg_l23, 2825000, 2825000);
#endif
		if (IS_ERR(reg_l23)) {
			printk(KERN_ERR"could not get 8941_l23, rc = %ld\n",
				PTR_ERR(reg_l23));
			return;
		}
	}

	if (onoff) {
		ret = regulator_enable(reg_l23);
		if (ret) {
			printk(KERN_ERR"enable l23 failed, rc=%d\n", ret);
			return;
		}
		printk(KERN_DEBUG"haptic power_on is finished.\n");
	} else {
		if (regulator_is_enabled(reg_l23)) {
			ret = regulator_disable(reg_l23);
			if (ret) {
				printk(KERN_ERR"disable l23 failed, rc=%d\n",
									ret);
				return;
			}
		}
		printk(KERN_DEBUG"haptic power_off is finished.\n");
	}
#else
	static struct regulator *reg_l17;

	if (!reg_l17) {
		reg_l17 = regulator_get(NULL, "8941_l17");
		ret = regulator_set_voltage(reg_l17, 3000000, 3000000);

		if (IS_ERR(reg_l17)) {
			printk(KERN_ERR"could not get 8941_l17, rc = %ld\n",
				PTR_ERR(reg_l17));
			return;
		}
	}

	if (onoff) {
		ret = regulator_enable(reg_l17);
		if (ret) {
			printk(KERN_ERR"enable l17 failed, rc=%d\n", ret);
			return;
		}
		printk(KERN_DEBUG"haptic power_on is finished.\n");
	} else {
		if (regulator_is_enabled(reg_l17)) {
			ret = regulator_disable(reg_l17);
			if (ret) {
				printk(KERN_ERR"disable l17 failed, rc=%d\n",
									ret);
				return;
			}
		}
		printk(KERN_DEBUG"haptic power_off is finished.\n");
	}
#endif
}
#endif


#if defined(CONFIG_MOTOR_DRV_DRV2603)
void drv2603_gpio_en(bool en)
{
	if (en) {
		gpio_direction_output(vibrator_drvdata.drv2603_en_gpio, 1);
	} else {
		gpio_direction_output(vibrator_drvdata.drv2603_en_gpio, 0);
	}
}
static int32_t drv2603_gpio_init(void)
{
	int ret;
	ret = gpio_request(vibrator_drvdata.drv2603_en_gpio, "vib enable");
	if (ret < 0) {
		printk(KERN_ERR "vib enable gpio_request is failed\n");
		return 1;
	}
	return 0;
}
#endif
#if defined(CONFIG_MOTOR_DRV_MAX77888)
void max77888_gpio_en(bool en)
{
	if (en) {
		gpio_direction_output(vibrator_drvdata.max77888_en_gpio, 1);
	} else {
		gpio_direction_output(vibrator_drvdata.max77888_en_gpio, 0);
	}
}
static int32_t max77888_gpio_init(void)
{
	int ret;
	ret = gpio_request(vibrator_drvdata.max77888_en_gpio, "vib enable");
	if (ret < 0) {
		printk(KERN_ERR "vib enable gpio_request is failed\n");
		return 1;
	}
	return 0;
}
#endif

static void vibrator_initialize(void)
{
	int ret;

	/* set gpio config	*/
	if (vibrator_drvdata.is_pmic_vib_pwm) { //PMIC PWM
		ret = gpio_request(vibrator_drvdata.vib_pwm_gpio, \
				"vib pwm");
		if (ret < 0) {
			printk(KERN_ERR"vib pwm gpio_request is failed\n");
			goto err2;
		}

		ret = pm8xxx_gpio_config(vibrator_drvdata.vib_pwm_gpio,\
				&vib_pwm);
		if (ret < 0) {
			printk(KERN_ERR "failed to configure vib pwm pmic gpio\n");
			goto err2;
		}
	} else { //AP PWM
#if defined(CONFIG_MACH_S3VE3G_EUR)
		gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,
					0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
#else
		gpio_tlmm_config(GPIO_CFG(vibrator_drvdata.vib_pwm_gpio,
					0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
#endif
	}
#if defined(CONFIG_MOTOR_ISA1000)
	if (!vibrator_drvdata.is_pmic_vib_en) {
		ret = gpio_request(vibrator_drvdata.vib_en_gpio,"vib enable");
		if (ret < 0) {
			printk(KERN_ERR "vib enable gpio_request is failed\n");
			goto err2;
		}
	}
#endif

#if defined(CONFIG_MOTOR_DRV_DRV2603)
	if (drv2603_gpio_init())
		goto err2;
#elif defined(CONFIG_MOTOR_DRV_MAX77888)
	if(max77888_gpio_init())
		goto err2;
#endif

	return;
err2:
	printk(KERN_ERR "%s failed check.\n", __func__);
}

static struct device *vib_dev;
extern struct class *sec_class;

static ssize_t show_vib_tuning(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sprintf(buf, "gp_m %d, gp_n %d, gp_d %d, pwm_mul %d, strength %d, min_str %d\n",
			g_nlra_gp_clk_m, g_nlra_gp_clk_n, g_nlra_gp_clk_d,
			g_nlra_gp_clk_pwm_mul, motor_strength, motor_min_strength);
	return strlen(buf);
}

static ssize_t store_vib_tuning(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	int temp_m, temp_n, temp_str;

	retval = sscanf(buf, "%d %d %d", &temp_m, &temp_n, &temp_str);
	if (retval == 0) {
		pr_info("[VIB]: %s, fail to get vib_tuning value\n", __func__);
		return count;
	}

	g_nlra_gp_clk_m = temp_m;
	g_nlra_gp_clk_n = temp_n;
	g_nlra_gp_clk_d = temp_n / 2;
	g_nlra_gp_clk_pwm_mul = temp_n;
	motor_strength = temp_str;
	motor_min_strength = g_nlra_gp_clk_n*MOTOR_MIN_STRENGTH/100;

	pr_info("[VIB]: %s gp_m %d, gp_n %d, gp_d %d, pwm_mul %d, strength %d, min_str %d\n", __func__,
			g_nlra_gp_clk_m, g_nlra_gp_clk_n, g_nlra_gp_clk_d,
			g_nlra_gp_clk_pwm_mul, motor_strength, motor_min_strength);

	return count;
}

static DEVICE_ATTR(vib_tuning, 0660, show_vib_tuning, store_vib_tuning);

static ssize_t intensity_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct timed_output_dev *t_dev = dev_get_drvdata(dev);
	struct ss_vib *vib = container_of(t_dev, struct ss_vib, timed_dev);
	int ret = 0, set_intensity = 0;

	ret = kstrtoint(buf, 0, &set_intensity);

	if ((set_intensity < 0) || (set_intensity > (MAX_INTENSITY / 100))) {
		pr_err("[VIB]: %sout of range\n", __func__);
		return -EINVAL;
	}

	vibe_set_intensity((set_intensity * 100));
	vib->intensity = (set_intensity * 100);

	return count;
}

static ssize_t intensity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct timed_output_dev *t_dev = dev_get_drvdata(dev);
	struct ss_vib *vib = container_of(t_dev, struct ss_vib, timed_dev);

	return sprintf(buf, "%u\n", (vib->intensity / 100));
}

static ssize_t pwm_default_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", (DEFAULT_INTENSITY / 100));
}

static ssize_t pwm_max_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", (MAX_INTENSITY / 100));
}

static ssize_t pwm_min_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", 0);
}

static ssize_t pwm_threshold_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", 75);
}

static DEVICE_ATTR(pwm_default, 0444, pwm_default_show, NULL);
static DEVICE_ATTR(pwm_max, 0444, pwm_max_show, NULL);
static DEVICE_ATTR(pwm_min, 0444, pwm_min_show, NULL);
static DEVICE_ATTR(pwm_threshold, 0444, pwm_threshold_show, NULL);
static DEVICE_ATTR(pwm_value, 0644, intensity_show, intensity_store);

static int ss_vibrator_probe(struct platform_device *pdev)
{
	struct ss_vib *vib;
	int rc = 0;

	pr_info("[VIB]: %s\n",__func__);

	motor_min_strength = g_nlra_gp_clk_n*MOTOR_MIN_STRENGTH/100;
	vib = devm_kzalloc(&pdev->dev, sizeof(*vib), GFP_KERNEL);
	if (!vib) {
		pr_err("[VIB]: %s : Failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	if (!pdev->dev.of_node) {
		pr_err("[VIB]: %s failed, DT is NULL", __func__);
		return -ENODEV;
	}

	vib->dev = &pdev->dev;
	rc = vibrator_parse_dt(vib);
	if(rc)
		return rc;

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || defined(CONFIG_MACH_JS01LTEDCM)
	virt_mmss_gp1_base = ioremap(MSM_MMSS_GP3_BASE,0x28);
#elif defined(CONFIG_SEC_BERLUTI_PROJECT) || defined(CONFIG_MACH_S3VE3G_EUR)
	virt_mmss_gp1_base = ioremap(MSM_MMSS_GP0_BASE,0x28);
#else
	virt_mmss_gp1_base = ioremap(MSM_MMSS_GP1_BASE,0x28);
#endif
	if (!virt_mmss_gp1_base)
		panic("[VIB]: Unable to ioremap MSM_MMSS_GP1 memory!");

#if defined(CONFIG_MOTOR_DRV_MAX77803) || defined(CONFIG_MOTOR_DRV_MAX77804K) || defined(CONFIG_MOTOR_DRV_MAX77828)
	vibrator_drvdata.power_onoff = max77803_haptic_power_onoff;
#else
	vibrator_drvdata.power_onoff = NULL;
#endif
	vibrator_drvdata.pwm_dev = NULL;

	vib->state = 0;
	vib->intensity = DEFAULT_INTENSITY;
	vib->timeout = VIB_DEFAULT_TIMEOUT;
	
	vibrator_initialize();
	vibe_set_intensity(vib->intensity);
	INIT_WORK(&vib->work, ss_vibrator_update);
	mutex_init(&vib->lock);

	vib->queue = create_singlethread_workqueue("ss_vibrator");
	if (!vib->queue) {
		pr_err("[VIB]: %s: can't create workqueue\n", __func__);
		return -ENOMEM;
	}

	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = vibrator_timer_func;

	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = vibrator_get_time;
	vib->timed_dev.enable = vibrator_enable;

	dev_set_drvdata(&pdev->dev, vib);

	rc = timed_output_dev_register(&vib->timed_dev);
	if (rc < 0) {
		pr_err("[VIB]: timed_output_dev_register fail (rc=%d)\n", rc);
		goto err_read_vib;
	}

	rc = sysfs_create_file(&vib->timed_dev.dev->kobj, &dev_attr_pwm_default.attr);
	rc = sysfs_create_file(&vib->timed_dev.dev->kobj, &dev_attr_pwm_min.attr);
	rc = sysfs_create_file(&vib->timed_dev.dev->kobj, &dev_attr_pwm_max.attr);
	rc = sysfs_create_file(&vib->timed_dev.dev->kobj, &dev_attr_pwm_threshold.attr);
	rc = sysfs_create_file(&vib->timed_dev.dev->kobj, &dev_attr_pwm_value.attr);
	if (rc < 0) {
		pr_err("[VIB]: Failed to register sysfs intensity: %d\n", rc);
	}

	vib_dev = device_create(sec_class, NULL, 0, NULL, "vib");
	if (IS_ERR(vib_dev)) {
		pr_info("[VIB]: Failed to create device for samsung vib\n");
	}

	rc = sysfs_create_file(&vib_dev->kobj, &dev_attr_vib_tuning.attr);
	if (rc) {
		pr_info("Failed to create sysfs group for samsung specific led\n");
	}

	return 0;
err_read_vib:
	iounmap(virt_mmss_gp1_base);
	destroy_workqueue(vib->queue);
	mutex_destroy(&vib->lock);
	return rc;
}

static int ss_vibrator_remove(struct platform_device *pdev)
{
	struct ss_vib *vib = dev_get_drvdata(&pdev->dev);
	iounmap(virt_mmss_gp1_base);

	destroy_workqueue(vib->queue);
	mutex_destroy(&vib->lock);

	return 0;
}

static const struct of_device_id vib_motor_match[] = {
	{	.compatible = "vibrator",
	},
	{}
};

static struct platform_driver ss_vibrator_platdrv =
{
	.driver =
	{
		.name = "vibrator",
		.owner = THIS_MODULE,
		.of_match_table = vib_motor_match,
		.pm	= &vibrator_pm_ops,
	},
	.probe = ss_vibrator_probe,
	.remove = ss_vibrator_remove,
};

static int __init ss_timed_vibrator_init(void)
{
	return platform_driver_register(&ss_vibrator_platdrv);
}

void __exit ss_timed_vibrator_exit(void)
{
	platform_driver_unregister(&ss_vibrator_platdrv);
}
module_init(ss_timed_vibrator_init);
module_exit(ss_timed_vibrator_exit);

MODULE_AUTHOR("Samsung Corporation");
MODULE_DESCRIPTION("timed output vibrator device");
MODULE_LICENSE("GPL v2");
