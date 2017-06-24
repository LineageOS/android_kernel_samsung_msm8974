/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
//#include <plat/iic.h>
//#include <plat/devs.h>
//#include <plat/gpio-cfg.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/err.h>
//#include <linux/pwm.h>
#include <linux/i2c-gpio.h>
#include <linux/isa1400_vibrator.h>
#include <linux/clk.h>

#define MOTOR_PWM_ID			0
#define MOTOR_PWM_PERIOD		485
#define MOTOR_MAX_TIMEOUT	10000
#define GPIO_MOTOR_EN 91
#define GPIO_MOTOR_SDA 33
#define GPIO_MOTOR_SCL 32
#define GPIO_PWM_CLK 27

//struct pwm_device	*motor_pwm;

static u8 isa1400_init[] = {
	ISA1400_REG_INIT, 21,
	ISA1400_REG_AMPGAIN1, 0x51,
	ISA1400_REG_AMPGAIN2, 0x51,
	ISA1400_REG_OVDRTYP, 0xca,
	ISA1400_REG_AMPSWTCH, 0x1a,
	ISA1400_REG_SYSCLK, 0x22,
	ISA1400_REG_GAIN, 0x00,
	ISA1400_REG_FREQ1M, 0x08,
	ISA1400_REG_FREQ1L, 0xD3,
	ISA1400_REG_CHMODE, 0x00,
	ISA1400_REG_WAVESEL, 0x01,
};

static u8 isa1400_start[] = {
	ISA1400_REG_START, 0,
	ISA1400_REG_GAIN, 0x7f,
	ISA1400_REG_HPTEN, 0x01,
};

static u8 isa1400_stop[] = {
	ISA1400_REG_STOP, 0,
	ISA1400_REG_GAIN, 0x00,
	ISA1400_REG_HPTEN, 0x00,
};

static const u8 *isa1400_reg_data[] = {
	isa1400_init,
	isa1400_start,
	isa1400_stop,
};

static int isa1400_vdd_en(bool en)
{
	int ret = 0;

	pr_info("[VIB] %s %s\n",
		__func__, en ? "on" : "off");

	ret = gpio_direction_output(GPIO_MOTOR_EN, en);

	if (en)
		usleep(400);

	return ret;
}

static int isa1400_clk_en(bool en)
{
	static struct clk *vib_src_clk = NULL;
	static struct clk *vib_gp1_clk = NULL;
	int err;

	if(!vib_src_clk){
		vib_src_clk = clk_get(NULL, "vib_src_clk");

		if(IS_ERR(vib_src_clk)) {
			pr_err("[VIB]%s: unable to get vib_clk \n", __func__);
			return -ENODEV;
		}
		clk_set_rate(vib_src_clk, 6000000);
	}
	if(!vib_gp1_clk){
		vib_gp1_clk = clk_get(NULL, "vib_gp1_clk");
		if(IS_ERR(vib_gp1_clk)) {
			pr_err("[VIB]%s: unable to get vib_clk2 \n", __func__);
			return -ENODEV;
		}
	}

        if (en) {
		err = clk_prepare_enable(vib_gp1_clk);
		gpio_tlmm_config(GPIO_CFG(GPIO_PWM_CLK,
				 6, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if(err){
			pr_err("[VIB] clk enable not succeeded %s\n", __func__);
			return err;
		}
        }
        else {
		clk_disable_unprepare(vib_gp1_clk);
		clk_put(vib_gp1_clk);
		clk_put(vib_src_clk);
		vib_src_clk = NULL;
		vib_gp1_clk = NULL;

		gpio_tlmm_config(GPIO_CFG(GPIO_PWM_CLK,
				 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(GPIO_PWM_CLK, 0);
        }
	return 0;
}

/*static struct i2c_gpio_platform_data gpio_i2c_data20 = {
	.sda_pin = GPIO_MOTOR_SDA,
	.scl_pin = GPIO_MOTOR_SCL,
};

struct platform_device vib_device_i2c20 __initdata = {
	.name = "i2c-gpio",
	.id = 20,
	.dev.platform_data = &gpio_i2c_data20,
};*/

static struct isa1400_vibrator_platform_data isa1400_vibrator_pdata = {
	.gpio_en = isa1400_vdd_en,
	.clk_en = isa1400_clk_en,
	.max_timeout = MOTOR_MAX_TIMEOUT,
	.reg_data = isa1400_reg_data,
};

static struct i2c_board_info i2c_devs20_emul[] __initdata = {
	{
		I2C_BOARD_INFO("isa1400_vibrator",  (0x90 >> 1)),
		.platform_data = &isa1400_vibrator_pdata,
	},
};

void __init vienna_motor_init(void)
{
	u32 gpio, gpio_motor_pwm;

	//pr_info("[VIB] system_rev %d\n", system_rev);
	printk("[VIB] %s\n", __func__);
	gpio = GPIO_MOTOR_EN;
	gpio_request(gpio, "MOTOR_EN");
	gpio_direction_output(gpio, 0);
	gpio_export(gpio, 0);
	gpio_motor_pwm = GPIO_PWM_CLK;
	gpio_request(gpio_motor_pwm, "MOTOR_CLK");
	gpio_tlmm_config(GPIO_CFG(gpio_motor_pwm,
			 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_direction_output(gpio_motor_pwm, 0);
	gpio_export(gpio_motor_pwm, 0);
	isa1400_init[1] = sizeof(isa1400_init);
	isa1400_start[1] = sizeof(isa1400_start);
	isa1400_stop[1] = sizeof(isa1400_stop);

	/*platform_device_register(&vib_device_i2c20);*/

	i2c_register_board_info(13, i2c_devs20_emul,
				ARRAY_SIZE(i2c_devs20_emul));
}
