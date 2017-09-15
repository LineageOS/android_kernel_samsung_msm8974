/*
 * drivers/media/isdbt/isdbt.c
 *
 * isdbt driver
 *
 * Copyright (C) (2014, Samsung Electronics)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/device.h>
#include <linux/string.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/pm_qos.h>
#include <linux/of_gpio.h>
#include "isdbt.h"

#if defined(CONFIG_MTV_QUALCOMM)
#include <mach/gpiomux.h>
#include <soc/qcom/pm.h>
#elif defined(CONFIG_MTV_EXYNOS)
#include <plat/gpio-cfg.h>
#elif defined(CONFIG_MTV_SPREADTRUM)
#include <mach/pinmap.h>
#endif

#if defined(ISDBT_USE_PMIC)
#include <linux/regulator/machine.h>
#endif

#define ISDBT_WAKE_LOCK_ENABLE
#ifdef ISDBT_WAKE_LOCK_ENABLE
#if defined(CONFIG_MTV_QUALCOMM)
static struct pm_qos_request isdbt_pm_qos_req;
#endif
static struct wake_lock isdbt_wlock;
#endif

static struct isdbt_drv_func *isdbtdrv_func = NULL;
static struct class *isdbt_class = NULL;
static struct isdbt_dt_platform_data *dt_pdata;
static bool isdbt_pwr_on = false;

#if defined(CONFIG_SEC_GPIO_SETTINGS)
static struct device  *isdbt_device = NULL;
#else
#if defined(CONFIG_MTV_QUALCOMM)
static struct gpiomux_setting spi_active_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting spi_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

#elif defined(CONFIG_MTV_SPREADTRUM)

#define SPI_PIN_FUNC_MASK  (0x3<<4)
#define SPI_PIN_FUNC_DEF   (0x0<<4)
#define SPI_PIN_FUNC_GPIO  (0x3<<4)

struct spi_pin_desc {
	const char   *name;
	unsigned int pin_func;
	unsigned int reg;
	unsigned int gpio;
};

static struct spi_pin_desc spi_pin_group[] = {
	{"SPI_MISO", SPI_PIN_FUNC_DEF, REG_PIN_SPI0_DI + CTL_PIN_BASE, 69},
	{"SPI_CLK", SPI_PIN_FUNC_DEF, REG_PIN_SPI0_CLK + CTL_PIN_BASE, 70},
	{"SPI_MOSI", SPI_PIN_FUNC_DEF, REG_PIN_SPI0_DO + CTL_PIN_BASE, 68},
	{"SPI_CS0", SPI_PIN_FUNC_GPIO, REG_PIN_SPI0_CSN + CTL_PIN_BASE, 67}
};
#endif
#endif

#if defined(ISDBT_USE_PMIC)
static void isdbt_control_pmic_pwr(bool on)
{
	static struct regulator *reg_isdbt = NULL;
	static int prev_on = 0;
	int rc = 0;

	if(on == prev_on)
		return;

	if(!reg_isdbt) {
		reg_isdbt = regulator_get(NULL, ISDBT_PMIC_NAME);
		if (IS_ERR(reg_isdbt)) {
			DPRINTK("%s : could not get %s\n", __func__, ISDBT_PMIC_NAME);
			reg_isdbt = NULL;
			return;
//			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_isdbt, ISDBT_PMIC_VOLTAGE, ISDBT_PMIC_VOLTAGE);
		if(rc) {
			DPRINTK("%s : set voltage failed for %s, rc = %d\n", __func__, ISDBT_PMIC_NAME, rc);
			return;
//			return -EINVAL;
		}
		DPRINTK("%s (get/set) : success\n", __func__);
	}

	if(on) {
		rc = regulator_enable(reg_isdbt);
		if(rc) {
			DPRINTK("%s : regulator enable failed, rc = %d\n", __func__, rc);
			return;
//			return rc;
		}
		DPRINTK("%s (on) : sunccess\n", __func__);
	} else {
		rc = regulator_disable(reg_isdbt);
		if(rc) {
			DPRINTK("%s : regulator disable failed, rc = %d\n", __func__, rc);
//			return rc;
		}
		else DPRINTK("%s (on) : success\n", __func__);
	}

	prev_on = on;

	return;
}
#endif

static void isdbt_gpio_set_output(void)
{
	gpio_direction_output(dt_pdata->isdbt_pwr_en, 0);
	if (gpio_is_valid(dt_pdata->isdbt_pwr_en2))
		gpio_direction_output(dt_pdata->isdbt_pwr_en2, 0);
	if (gpio_is_valid(dt_pdata->isdbt_rst))
		gpio_direction_output(dt_pdata->isdbt_rst, 0);
	gpio_direction_input(dt_pdata->isdbt_irq);
}

static void isdbt_gpio_set_input(void)
{
	gpio_direction_input(dt_pdata->isdbt_pwr_en);
	if (gpio_is_valid(dt_pdata->isdbt_pwr_en2))
		gpio_direction_input(dt_pdata->isdbt_pwr_en2);
	if (gpio_is_valid(dt_pdata->isdbt_rst))
		gpio_direction_input(dt_pdata->isdbt_rst);
}

static void isdbt_gpio_config(bool poweron)
{
#if defined(CONFIG_SEC_GPIO_SETTINGS)
	struct pinctrl *isdbt_pinctrl;

	/* Get pinctrl if target uses pinctrl */
	isdbt_pinctrl = (poweron ? devm_pinctrl_get_select(isdbt_device, "isdbt_gpio_active") : devm_pinctrl_get_select(isdbt_device, "isdbt_gpio_suspend"));
	if (IS_ERR(isdbt_pinctrl)) {
		DPRINTK("Target does not use pinctrl\n");
		isdbt_pinctrl = NULL;
	}
#else
#if defined(CONFIG_MTV_QUALCOMM)

	if(poweron) {
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_mosi, GPIOMUX_ACTIVE, &spi_active_config, NULL) < 0)
			DPRINTK("spi_mosi Port request error!!!\n");
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_miso, GPIOMUX_ACTIVE, &spi_active_config, NULL) < 0)
			DPRINTK("spi_miso Port request error!!!\n");
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_cs, GPIOMUX_ACTIVE, &spi_active_config, NULL) < 0)
			DPRINTK("spi_cs Port request error!!!\n");
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_clk, GPIOMUX_ACTIVE, &spi_active_config, NULL) < 0)
			DPRINTK("spi_clk Port request error!!!\n");
	} else {
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_mosi, GPIOMUX_SUSPENDED, &spi_suspend_config, NULL) < 0)
			DPRINTK("spi_mosi Port request error!!!\n");
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_miso, GPIOMUX_SUSPENDED, &spi_suspend_config, NULL) < 0)
			DPRINTK("spi_miso Port request error!!!\n");
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_cs, GPIOMUX_SUSPENDED, &spi_suspend_config, NULL) < 0)
			DPRINTK("spi_cs Port request error!!!\n");
		if(msm_gpiomux_write(dt_pdata->isdbt_spi_clk, GPIOMUX_SUSPENDED, &spi_suspend_config, NULL) < 0)
			DPRINTK("spi_clk Port request error!!!\n");
	}

#elif defined(CONFIG_MTV_BROADCOM)

	if(poweron) {
		struct pin_config SdioPinCfgs;
		
		SdioPinCfgs.name = dt_pdata->isdbt_irq;
		pinmux_get_pin_config(&SdioPinCfgs);
		SdioPinCfgs.reg.b.slew_rate_ctrl = 0;
		pinmux_set_pin_config(&SdioPinCfgs);

		SdioPinCfgs.name = dt_pdata->isdbt_rst;
		pinmux_get_pin_config(&SdioPinCfgs);
		SdioPinCfgs.reg.b.input_dis = 1;
		SdioPinCfgs.reg.b.drv_sth = 0;
		SdioPinCfgs.func = PF_GPIO00;
		SdioPinCfgs.reg.b.sel = 4;	
		pinmux_set_pin_config(&SdioPinCfgs);
	}

#elif defined(CONFIG_MTV_SPREADTRUM)

	if(poweron) {
		unsigned int reg, gpio, pin_func, value;
		unsigned long flags;
		int i, regs_count;

		regs_count = sizeof(spi_pin_group)/sizeof(struct spi_pin_desc);

		for (i = 0; i < regs_count; i++) {
			pin_func = spi_pin_group[i].pin_func;
			gpio = spi_pin_group[i].gpio;

			if (pin_func == SPI_PIN_FUNC_DEF) {
				reg = spi_pin_group[i].reg;
				/* free the gpios that have request */
	//			gpio_free(gpio);
				local_irq_save(flags);
				/* config pin default spi function */
				value = ((__raw_readl(reg) & ~SPI_PIN_FUNC_MASK) | SPI_PIN_FUNC_DEF);
				__raw_writel(value, reg);
				local_irq_restore(flags);
			}
			else {
				/* CS should config output */
				gpio_direction_output(gpio, 1);
			}
		}
	}
#endif
#endif

	if(poweron)
		isdbt_gpio_set_output();
	else
		isdbt_gpio_set_input();
}

static void isdbt_gpio_on(void)
{
	DPRINTK("isdbt_gpio_on\n");

//	isdbt_gpio_config(true);
#if defined(ISDBT_USE_PMIC)
	isdbt_control_pmic_pwr(true);
#endif

	if (gpio_is_valid(dt_pdata->isdbt_pwr_en2))
	{
		gpio_set_value(dt_pdata->isdbt_pwr_en2, 1);
		usleep_range(1000, 1000);
	}
	gpio_set_value(dt_pdata->isdbt_pwr_en, 1);
	usleep_range(1000, 1000);

	if (gpio_is_valid(dt_pdata->isdbt_rst)) {
		gpio_set_value(dt_pdata->isdbt_rst, 1);
		usleep_range(1000, 1000);
	}
}

static void isdbt_gpio_off(void)
{
	DPRINTK("isdbt_gpio_off\n");

//	isdbt_gpio_config(false);
#if defined(ISDBT_USE_PMIC)
	isdbt_control_pmic_pwr(false);
#endif

	gpio_set_value(dt_pdata->isdbt_pwr_en, 0);
	usleep_range(1000, 1000);
	if (gpio_is_valid(dt_pdata->isdbt_pwr_en2)) {
		gpio_set_value(dt_pdata->isdbt_pwr_en2, 0);
		usleep_range(1000, 1000);
	}
	if (gpio_is_valid(dt_pdata->isdbt_rst)) {
		gpio_set_value(dt_pdata->isdbt_rst, 0);
	}
}

 bool isdbt_power_off(void)
{
	DPRINTK("%s : isdbt_power_off(%d)\n", __func__, isdbt_pwr_on);

	if (isdbt_pwr_on) {
		(*isdbtdrv_func->power_off)();

#ifdef ISDBT_WAKE_LOCK_ENABLE
		wake_unlock(&isdbt_wlock);
#if defined(CONFIG_MTV_QUALCOMM)
		pm_qos_update_request(&isdbt_pm_qos_req, PM_QOS_DEFAULT_VALUE);
#endif
#endif

		isdbt_pwr_on = false;
	}

	return true;
}

int isdbt_power_on(unsigned long arg)
{
	int ret;

	ret = (*isdbtdrv_func->power_on)(arg);

#ifdef ISDBT_WAKE_LOCK_ENABLE
#if defined(CONFIG_MTV_QUALCOMM)
	pm_qos_update_request(&isdbt_pm_qos_req,
				  msm_cpuidle_get_deep_idle_latency());
#endif
	wake_lock(&isdbt_wlock);
#endif

	if (ret == 0)
		isdbt_pwr_on = true;
	else
		isdbt_pwr_on = false;

	DPRINTK("%s : ret(%d)\n", __func__, isdbt_pwr_on);
	return ret;
}

bool isdbt_control_irq(bool set)
{
	bool ret = true;
	int irq_ret;

	DPRINTK("isdbt_control_irq\n");

	if (!gpio_is_valid(dt_pdata->isdbt_irq))
		return false;

	if (set) {
		irq_ret = request_threaded_irq(gpio_to_irq(dt_pdata->isdbt_irq), NULL,
									isdbtdrv_func->irq_handler,
								#if defined(CONFIG_MTV_FC8300) || defined(CONFIG_MTV_FC8150)
									IRQF_DISABLED | IRQF_TRIGGER_RISING | IRQF_ONESHOT,
								#elif defined(CONFIG_MTV_MTV222) || defined(CONFIG_MTV_MTV23x)
									IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
								#endif
									ISDBT_DEV_NAME, NULL);
		if (irq_ret < 0) {
			DPRINTK("request_irq failed !! \r\n");
			ret = false;
		}
	} else {
		free_irq(gpio_to_irq(dt_pdata->isdbt_irq), NULL);
	}

	return ret;
}

void isdbt_control_gpio(bool poweron)
{
	if (poweron)
		isdbt_gpio_on();
	else
		isdbt_gpio_off();
}

static ssize_t isdbt_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
	int ret = -EFAULT;
//	DPRINTK("isdbt_read\n");

	if (isdbtdrv_func->read)
		ret = (*isdbtdrv_func->read)(filp, buf, count, pos);
	else
		ret = 0;

	return ret;
}

static long isdbt_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = -EFAULT;

	if (isdbtdrv_func->ioctl)
		ret = (*isdbtdrv_func->ioctl)(filp, cmd, arg);

	return ret;
}

static int isdbt_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = -EFAULT;

	if (isdbtdrv_func->mmap)
		ret = (*isdbtdrv_func->mmap)(filp, vma);

	return ret;
}

static int isdbt_release(struct inode *inode, struct file *filp)
{
	int ret = -EFAULT;

	DPRINTK("isdbt_release\n");

	if (isdbtdrv_func->release)
		ret = (*isdbtdrv_func->release)(inode, filp);

	isdbt_power_off();
	isdbt_gpio_config(false);
	isdbt_control_irq(false);

	return 0;
}

static int isdbt_open(struct inode *inode, struct file *filp)
{
	int ret = -EFAULT;
	DPRINTK("isdbt_open\n");

	isdbt_gpio_config(true);
	isdbt_control_irq(true);

	if (isdbtdrv_func->open)
		ret = (*isdbtdrv_func->open)(inode, filp);
	else
		ret = 0;

	return ret;
}

static const struct file_operations isdbt_ctl_fops = {
	.owner          = THIS_MODULE,
	.open           = isdbt_open,
	.read           = isdbt_read,
	.unlocked_ioctl  = isdbt_ioctl,
	.mmap           = isdbt_mmap,
	.release	    = isdbt_release,
	.llseek         = no_llseek,
};

static bool get_isdbt_dt_pdata(struct device *dev)
{
	dt_pdata = devm_kzalloc(dev, sizeof(struct isdbt_dt_platform_data), GFP_KERNEL);
	if (!dt_pdata) {
		DPRINTK("could not allocate memory for platform data\n");
		goto err;
	}

	if(dev->of_node)
	{
		dt_pdata->isdbt_pwr_en = of_get_named_gpio(dev->of_node, "isdbt_pwr_en", 0);
		if (dt_pdata->isdbt_pwr_en < 0) {
			DPRINTK("can not find the isdbt_pwr_en\n");
			goto alloc_err;
		}
		dt_pdata->isdbt_pwr_en2 = of_get_named_gpio(dev->of_node, "isdbt_pwr_en2", 0);
		if (dt_pdata->isdbt_pwr_en2 < 0) {
			DPRINTK("can not find the isdbt_pwr_en2\n");
			//goto alloc_err;
		}
		dt_pdata->isdbt_rst = of_get_named_gpio(dev->of_node, "isdbt_rst", 0);
		if (dt_pdata->isdbt_rst < 0) {
			DPRINTK("can not find the isdbt_rst\n");
			//goto alloc_err;
		}
		dt_pdata->isdbt_irq = of_get_named_gpio(dev->of_node, "isdbt_irq", 0);
		if (dt_pdata->isdbt_irq < 0) {
			DPRINTK("can not find the isdbt_irq\n");
			goto alloc_err;
		}
		dt_pdata->isdbt_spi_mosi = of_get_named_gpio(dev->of_node, "isdbt_spi_mosi", 0);
		if (dt_pdata->isdbt_spi_mosi < 0) {
			DPRINTK("can not find the isdbt_spi_mosi\n");
			goto alloc_err;
		}
		dt_pdata->isdbt_spi_miso = of_get_named_gpio(dev->of_node, "isdbt_spi_miso", 0);
		if (dt_pdata->isdbt_spi_miso < 0) {
			DPRINTK("can not find the isdbt_spi_miso\n");
			goto alloc_err;
		}
		dt_pdata->isdbt_spi_cs = of_get_named_gpio(dev->of_node, "isdbt_spi_cs", 0);
		if (dt_pdata->isdbt_spi_cs < 0) {
			DPRINTK("can not find the isdbt_spi_cs\n");
			goto alloc_err;
		}
		dt_pdata->isdbt_spi_clk = of_get_named_gpio(dev->of_node, "isdbt_spi_clk", 0);
		if (dt_pdata->isdbt_spi_clk < 0) {
			DPRINTK("can not find the isdbt_spi_clk\n");
			goto alloc_err;
		}
	}
	else {
		DPRINTK("could find device tree\n");
		dt_pdata->isdbt_pwr_en = convert_gpio(ISDBT_PWR_EN);
		dt_pdata->isdbt_pwr_en2 = convert_gpio(ISDBT_PWR_EN2);
		dt_pdata->isdbt_rst = convert_gpio(ISDBT_RST);
		dt_pdata->isdbt_irq = convert_gpio(ISDBT_INT);
		dt_pdata->isdbt_spi_mosi = convert_gpio(ISDBT_SPI_MOSI);
		dt_pdata->isdbt_spi_miso = convert_gpio(ISDBT_SPI_MISO);
		dt_pdata->isdbt_spi_cs = convert_gpio(ISDBT_SPI_CS);
		dt_pdata->isdbt_spi_clk = convert_gpio(ISDBT_SPI_CLK);
	}
	return true;

alloc_err:
	devm_kfree(dev, dt_pdata);

err:
	return false;
}

/*
 * isdbt_gpio_request
 * @ return value : It returns zero on success, else an error.
*/
static int isdbt_gpio_request(void)
{
	int err = 0;

	/* gpio for isdbt_pwr_en */
	err = gpio_request(dt_pdata->isdbt_pwr_en, "isdbt_pwr_en");
	if (err) {
		DPRINTK("isdbt_pwr_en: gpio request failed\n");
		goto out;
	}

	/* gpio for isdbt_pwr_en2 */
	if (gpio_is_valid(dt_pdata->isdbt_pwr_en2))
	{
		err = gpio_request(dt_pdata->isdbt_pwr_en2, "isdbt_pwr_en2");
		if (err) {
			DPRINTK("isdbt_pwr_en2: gpio request failed\n");
			goto out;
		}
	}

	/* gpio for isdbt_rst */
	if (gpio_is_valid(dt_pdata->isdbt_rst))
	{
		err = gpio_request(dt_pdata->isdbt_rst, "isdbt_rst");
		if (err) {
			DPRINTK("isdbt_rst: gpio request failed\n");
			goto out;
		}
	}

	/* gpio for isdbt_irq */
	err = gpio_request(dt_pdata->isdbt_irq, "isdbt_irq");
	if (err) {
		DPRINTK("isdbt_irq: gpio request failed\n");
		goto out;
	}

out:
	return err;
}

static struct isdbt_drv_func *isdbt_get_drv_func(void)
{
	struct isdbt_drv_func * (*func)(void);

#if defined(CONFIG_MTV_FC8150)
	func = fc8150_drv_func;
#elif defined(CONFIG_MTV_FC8300)
	func = fc8300_drv_func;
#elif defined(CONFIG_MTV_MTV222)
	func = mtv222_drv_func;
#elif defined(CONFIG_MTV_MTV23x)
	func = mtv23x_drv_func;
#else
	#error "an unsupported ISDBT driver!!!"
#endif
	return func();
}

static int isdbt_probe(struct platform_device *pdev)
{
	int ret = 0;
	int result = 0;

	DPRINTK("isdbt_probe\n");

	result = get_isdbt_dt_pdata(&pdev->dev);
	if (!result) {
		DPRINTK("isdbt_dt_pdata is NULL.\n");
		return -ENODEV;
	}

	result = isdbt_gpio_request();
	if (result) {
		DPRINTK("can't request gpio. please check the isdbt_gpio_request()");
		return -ENODEV;
	}

	isdbt_gpio_config(false);

	isdbt_spi_init();

	isdbtdrv_func = isdbt_get_drv_func();

	if (isdbtdrv_func->probe) {
		if ((*isdbtdrv_func->probe)() != 0) {
//			isdbt_exit_bus();
			ret = -EFAULT;
		}
	} else {
		pr_err("%s : isdbtdrv_func is NULL.\n", __func__);
//		isdbt_exit_bus();
		ret = -EFAULT;
	}

#ifdef ISDBT_WAKE_LOCK_ENABLE
#if defined(CONFIG_MTV_QUALCOMM)
	pm_qos_add_request(&isdbt_pm_qos_req, PM_QOS_CPU_DMA_LATENCY,
				PM_QOS_DEFAULT_VALUE);
#endif
	wake_lock_init(&isdbt_wlock, WAKE_LOCK_SUSPEND, "isdbt_wlock");
#endif

	return ret;
}

static int isdbt_remove(struct platform_device *pdev)
{
	int ret = 0;

	DPRINTK("isdbt_remove!\n");

#ifdef ISDBT_WAKE_LOCK_ENABLE
#if defined(CONFIG_MTV_QUALCOMM)
	pm_qos_remove_request(&isdbt_pm_qos_req);
#endif
	wake_lock_destroy(&isdbt_wlock);
#endif

	if (isdbtdrv_func->remove)
		ret = isdbtdrv_func->remove();

//	isdbt_gpio_config(false);

	return 0;
}

static int isdbt_suspend(struct platform_device *pdev, pm_message_t mesg)
{
	return 0;
}

static int isdbt_resume(struct platform_device *pdev)
{
	return 0;
}

#ifdef ISDBT_DEVICE_TREE
static const struct of_device_id isdbt_match_table[] = {
	{   .compatible = "isdbt_pdata",
	},
	{}
};
#endif
static struct platform_driver isdbt_driver = {
	.probe	= isdbt_probe,
	.remove = isdbt_remove,
	.suspend = isdbt_suspend,
	.resume = isdbt_resume,
	.driver = {
		.owner	= THIS_MODULE,
		.name = "isdbt",
#ifdef ISDBT_DEVICE_TREE
		.of_match_table = isdbt_match_table,
#endif
	},
};

static int __init isdbt_init(void)
{
   	int ret;
	struct device *isdbt_dev;

	DPRINTK("Module init\n");

	ret = register_chrdev(ISDBT_DEV_MAJOR, ISDBT_DEV_NAME, &isdbt_ctl_fops);
	if (ret < 0) {
		DPRINTK("Failed to register chrdev\n");
		return ret;
	}

	isdbt_class = class_create(THIS_MODULE, ISDBT_DEV_NAME);
	if (IS_ERR(isdbt_class)) {
		DPRINTK("class_create failed!\n");
		ret = -EFAULT;
		goto unreg_chrdev;
	}

	isdbt_dev = device_create(isdbt_class, NULL,
				MKDEV(ISDBT_DEV_MAJOR, ISDBT_DEV_MINOR),
				NULL, ISDBT_DEV_NAME);
	if (IS_ERR(isdbt_dev)) {
		ret = -EFAULT;
		DPRINTK("device_create failed!\n");
		goto destory_class;
	}

	ret = platform_driver_register(&isdbt_driver);
	if (ret) {
		DPRINTK("platform_driver_register failed!\n");
		goto destory_device;
	}

	return 0;

destory_device:
	device_destroy(isdbt_class, MKDEV(ISDBT_DEV_MAJOR, ISDBT_DEV_MINOR));

destory_class:
	class_destroy(isdbt_class);

unreg_chrdev:
	unregister_chrdev(ISDBT_DEV_MAJOR, ISDBT_DEV_NAME);

	return ret;
}

static void __exit isdbt_exit(void)
{
	DPRINTK("Module exit\n");

	platform_driver_unregister(&isdbt_driver);

	isdbt_spi_exit();

	device_destroy(isdbt_class, MKDEV(ISDBT_DEV_MAJOR, ISDBT_DEV_MINOR));

	class_destroy(isdbt_class);

	unregister_chrdev(ISDBT_DEV_MAJOR, ISDBT_DEV_NAME);
}

module_init(isdbt_init);
module_exit(isdbt_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("ISDBT driver");
MODULE_LICENSE("GPL v2");

