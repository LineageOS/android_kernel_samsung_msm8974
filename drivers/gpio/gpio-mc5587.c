/*
 * GPIO Chip driver for Analog Devices
 * MC5587 I/O Expander and QWERTY Keypad Controller
 *
 * Copyright 2009-2010 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>

#include <linux/i2c/mc5587.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>

#if defined(CONFIG_OF)
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#endif

#define DRV_NAME	"mc5587-gpio"

/*
 * Early pre 4.0 Silicon required to delay readout by at least 25ms,
 * since the Event Counter Register updated 25ms after the interrupt
 * asserted.
 */
#define WA_DELAYED_READOUT_REVID(rev)	((rev) < 4)

struct mc5587_gpio {
	struct i2c_client *client;
	struct gpio_chip gpio_chip;
	struct mutex lock;	/* protect cached dir, dat_out */
	/* protect serialized access to the interrupt controller bus */
	struct mutex irq_lock;
	unsigned gpio_start;
	int irq;
	int irq_base;
	int irq_gpio;
	uint8_t dat_out[3];
	uint8_t dir[3];
	uint8_t int_lvl[3];
	uint8_t int_en[3];
	uint8_t irq_mask[3];
	uint8_t irq_stat[3];
};

static int mc5587_read(struct i2c_client *client, u8 reg)
{
	int ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "Read Error [%d]\n", ret);

	return ret;
}

static int mc5587_write(struct i2c_client *client, u8 reg, u8 val)
{
	int ret = i2c_smbus_write_byte_data(client, reg, val);

	if (ret < 0)
		dev_err(&client->dev, "Write Error [%d]\n", ret);

	return ret;
}

static int mc5587_gpio_get_value(struct gpio_chip *chip, unsigned off)
{
	struct mc5587_gpio *dev =
	    container_of(chip, struct mc5587_gpio, gpio_chip);

	return !!(mc5587_read(dev->client,
		  GPIDAT1 + MC5587_BANK(off)) & MC5587_BIT(off));
}

static void mc5587_gpio_set_value(struct gpio_chip *chip,
				   unsigned off, int val)
{
	unsigned bank, bit;
	struct mc5587_gpio *dev =
	    container_of(chip, struct mc5587_gpio, gpio_chip);

	pr_info("%s\n", __func__);
	bank = MC5587_BANK(off);
	bit = MC5587_BIT(off);

	mutex_lock(&dev->lock);
	if (val)
		dev->dat_out[bank] |= bit;
	else
		dev->dat_out[bank] &= ~bit;

	mc5587_write(dev->client, GPODAT1 + bank,
			   dev->dat_out[bank]);
	mutex_unlock(&dev->lock);
}

static int mc5587_gpio_direction_input(struct gpio_chip *chip, unsigned off)
{
	int ret;
	unsigned bank;
	struct mc5587_gpio *dev =
	    container_of(chip, struct mc5587_gpio, gpio_chip);

	pr_info("%s\n", __func__);
	bank = MC5587_BANK(off);

	mutex_lock(&dev->lock);
	dev->dir[bank] &= ~MC5587_BIT(off);
	ret = mc5587_write(dev->client, GPIODIR1 + bank, dev->dir[bank]);
	mutex_unlock(&dev->lock);

	return ret;
}

static int mc5587_gpio_direction_output(struct gpio_chip *chip,
					 unsigned off, int val)
{
	int ret;
	unsigned bank, bit;
	struct mc5587_gpio *dev =
	    container_of(chip, struct mc5587_gpio, gpio_chip);

	pr_info("%s\n", __func__);
	bank = MC5587_BANK(off);
	bit = MC5587_BIT(off);

	mutex_lock(&dev->lock);
	dev->dir[bank] |= bit;

	if (val)
		dev->dat_out[bank] |= bit;
	else
		dev->dat_out[bank] &= ~bit;

	ret = mc5587_write(dev->client, GPODAT1 + bank,
				 dev->dat_out[bank]);
	ret |= mc5587_write(dev->client, GPIODIR1 + bank,
				 dev->dir[bank]);
	mutex_unlock(&dev->lock);

	return ret;
}

static int mc5587_gpio_request(struct gpio_chip *chip, unsigned off)
{
	struct mc5587_gpio *dev =
		container_of(chip, struct mc5587_gpio, gpio_chip);

	pr_info("%s\n", __func__);
	if (off >= dev->gpio_chip.ngpio) {
		pr_err("[%s] offset over max = [%d]\n", __func__, off);
		return 1;
	}
	/* to do*/
	return 0;
}

static void mc5587_gpio_free(struct gpio_chip *chip, unsigned off)
{
	struct mc5587_gpio *dev =
		container_of(chip, struct mc5587_gpio, gpio_chip);

	pr_info("%s\n", __func__);
	if (off >= dev->gpio_chip.ngpio) {
		pr_err("[%s] offset over max = [%d]\n", __func__, off);
	}
	/* to do*/
}

#ifdef CONFIG_GPIO_MC5587_IRQ
static int mc5587_gpio_to_irq(struct gpio_chip *chip, unsigned off)
{
	struct mc5587_gpio *dev =
		container_of(chip, struct mc5587_gpio, gpio_chip);
	return dev->irq_base + off;
}

static void mc5587_irq_bus_lock(struct irq_data *d)
{
	struct mc5587_gpio *dev = irq_data_get_irq_chip_data(d);

	mutex_lock(&dev->irq_lock);
}

 /*
  * genirq core code can issue chip->mask/unmask from atomic context.
  * This doesn't work for slow busses where an access needs to sleep.
  * bus_sync_unlock() is therefore called outside the atomic context,
  * syncs the current irq mask state with the slow external controller
  * and unlocks the bus.
  */

static void mc5587_irq_bus_sync_unlock(struct irq_data *d)
{
	struct mc5587_gpio *dev = irq_data_get_irq_chip_data(d);
	int i;

	for (i = 0; i <= MC5587_BANK(MC5587_MAXGPIO); i++)
		if (dev->int_en[i] ^ dev->irq_mask[i]) {
			dev->int_en[i] = dev->irq_mask[i];
			mc5587_write(dev->client, GPINTEN1 + i,
					   dev->int_en[i]);
		}

	mutex_unlock(&dev->irq_lock);
}

static void mc5587_irq_mask(struct irq_data *d)
{
	struct mc5587_gpio *dev = irq_data_get_irq_chip_data(d);
	unsigned gpio = d->irq - dev->irq_base;

	dev->irq_mask[MC5587_BANK(gpio)] &= ~MC5587_BIT(gpio);
}

static void mc5587_irq_unmask(struct irq_data *d)
{
	struct mc5587_gpio *dev = irq_data_get_irq_chip_data(d);
	unsigned gpio = d->irq - dev->irq_base;

	dev->irq_mask[MC5587_BANK(gpio)] |= MC5587_BIT(gpio);
}

static int mc5587_irq_set_type(struct irq_data *d, unsigned int type)
{
	struct mc5587_gpio *dev = irq_data_get_irq_chip_data(d);
	uint16_t gpio = d->irq - dev->irq_base;
	unsigned bank, bit;

	if ((type & IRQ_TYPE_EDGE_BOTH)) {
		dev_err(&dev->client->dev, "irq %d: unsupported type %d\n",
			d->irq, type);
		return -EINVAL;
	}

	bank = MC5587_BANK(gpio);
	bit = MC5587_BIT(gpio);

	if (type & IRQ_TYPE_LEVEL_HIGH)
		dev->int_lvl[bank] |= bit;
	else if (type & IRQ_TYPE_LEVEL_LOW)
		dev->int_lvl[bank] &= ~bit;
	else
		return -EINVAL;

	mc5587_gpio_direction_input(&dev->gpio_chip, gpio);
	mc5587_write(dev->client, GPINTLVL1 + bank,
			   dev->int_lvl[bank]);

	return 0;
}

static struct irq_chip mc5587_irq_chip = {
	.name			= "mc5587",
	.irq_mask		= mc5587_irq_mask,
	.irq_unmask		= mc5587_irq_unmask,
	.irq_bus_lock		= mc5587_irq_bus_lock,
	.irq_bus_sync_unlock	= mc5587_irq_bus_sync_unlock,
	.irq_set_type		= mc5587_irq_set_type,
};

static int mc5587_read_intstat(struct i2c_client *client, u8 *buf)
{
	int ret = i2c_smbus_read_i2c_block_data(client, GPINTST1, 3, buf);

	if (ret < 0)
		dev_err(&client->dev, "Read INT_STAT Error\n");

	return ret;
}

static irqreturn_t mc5587_irq_handler(int irq, void *devid)
{
	struct mc5587_gpio *dev = devid;
	unsigned status, bank, bit, pending;
	int ret;
	status = mc5587_read(dev->client, INTPND);

	if (status & MC5587_GPIPND) {
		ret = mc5587_read_intstat(dev->client, dev->irq_stat);
		if (ret < 0)
			memset(dev->irq_stat, 0, ARRAY_SIZE(dev->irq_stat));

		for (bank = 0; bank <= MC5587_BANK(MC5587_MAXGPIO);
			bank++, bit = 0) {
			pending = dev->irq_stat[bank] & dev->irq_mask[bank];

			while (pending) {
				if (pending & (1 << bit)) {
					handle_nested_irq(dev->irq_base +
							  (bank << 3) + bit);
					pending &= ~(1 << bit);

				}
				bit++;
			}
		}
	}

	mc5587_write(dev->client, INTPND, status); /* Status is W1C */

	return IRQ_HANDLED;
}

static int mc5587_irq_setup(struct mc5587_gpio *dev)
{
	struct i2c_client *client = dev->client;
	struct mc5587_gpio_platform_data *pdata = client->dev.platform_data;
	unsigned gpio;
	int ret, cur_irq;

	mc5587_write(client, CFGREG, MC5587_AUTOINC);
	mc5587_write(client, INTPND, -1); /* status is W1C */
	mc5587_read_intstat(client, dev->irq_stat); /* read to clear */

	dev->irq_gpio = pdata->irq_gpio;
	dev->irq_base = pdata->irq_base;

	mutex_init(&dev->irq_lock);

	dev->irq = gpio_to_irq(dev->irq_gpio);
	client->irq = dev->irq;
	pr_info("[%s] dev irq=[%d], irq_gpio=[%d] irq_base=[%d]\n",
			__func__, dev->irq, dev->irq_gpio, dev->irq_base);

	ret = gpio_request(dev->irq_gpio, "mc5587_expander_irq");
	if (ret) {
		dev_err(&client->dev, "[%s] failed requesting gpio %d\n",
				__func__, dev->irq_gpio);
		return ret;
	}
	gpio_direction_input(dev->irq_gpio);
	gpio_free(dev->irq_gpio);

	for (gpio = 0; gpio < dev->gpio_chip.ngpio; gpio++) {
		cur_irq = gpio + dev->irq_base;
		irq_set_chip_data(cur_irq, dev);
		irq_set_chip_and_handler(cur_irq, &mc5587_irq_chip,
					 handle_level_irq);
		irq_set_nested_thread(cur_irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(cur_irq, IRQF_VALID);
#else
		irq_set_noprobe(irq);
#endif
	}

	ret = request_threaded_irq(dev->irq,
				   NULL, mc5587_irq_handler,
				   IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				   "mc5587-irq", dev);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
			client->irq);
		goto out;
	}

	mc5587_write(client, CFGREG,
		MC5587_AUTOINC | MC5587_INTCFG | MC5587_GPIIEN);

	return 0;
out:
	dev->irq_base = 0;
	return ret;
}

static void mc5587_irq_teardown(struct mc5587_gpio *dev)
{
	if (dev->irq_base)
		free_irq(dev->client->irq, dev);
}

#else
static int mc5587_irq_setup(struct mc5587_gpio *dev)
{
	struct i2c_client *client = dev->client;
	dev_warn(&client->dev, "interrupt support not compiled in\n");

	return 0;
}

static void mc5587_irq_teardown(struct mc5587_gpio *dev)
{
}
#endif /* CONFIG_GPIO_MC5587_IRQ */

static int __devinit mc5587_gpio_setup(struct mc5587_gpio *dev)
{
	int ret = 0;
	unsigned bank = 0, bit = 0x00;

	ret |= mc5587_write(dev->client, CFGREG,
			MC5587_AUTOINC | MC5587_GPIIEN);
	ret |= mc5587_write(dev->client, INTPND, MC5587_GPIPND);

	ret |= mc5587_write(dev->client, KPMSEL1, 0x00);
	ret |= mc5587_write(dev->client, KPMSEL2, 0x00);
	ret |= mc5587_write(dev->client, KPMSEL3, 0x00);

	ret |= mc5587_write(dev->client, GPINTEN1, 0x00);
	ret |= mc5587_write(dev->client, GPINTEN2,
			MC5587_BIT(GPI_PIN_CAM_SENSOR_DET)
			| MC5587_BIT(GPI_PIN_TX_GTR_THRES));
	ret |= mc5587_write(dev->client, GPINTEN3, 0x00);

	ret |= mc5587_write(dev->client, GPINTLVL2,
			MC5587_BIT(GPI_PIN_CAM_SENSOR_DET)
			| MC5587_BIT(GPI_PIN_TX_GTR_THRES));

	ret |= mc5587_write(dev->client, GPIEMSEL1, 0x00);
	ret |= mc5587_write(dev->client, GPIEMSEL2, 0x00);
	ret |= mc5587_write(dev->client, GPIEMSEL3, 0x00);

	ret |= mc5587_write(dev->client, DBNCON1, 0xFF);
	ret |= mc5587_write(dev->client, DBNCON2, 0xFF);
	ret |= mc5587_write(dev->client, DBNCON3, 0xFF);

	bank = 0;
	bit = 0x01;	/* 0b00000001 */
	dev->dir[bank] = bit;
	dev->dat_out[bank] = 0x00;
	ret |= mc5587_write(dev->client, GPIODIR1, dev->dir[bank]);
	ret |= mc5587_write(dev->client, GPODAT1, dev->dat_out[bank]);

	bank = 1;
	bit = 0xed;	/* 0b11101101 */
	dev->dir[bank] = bit;
	dev->dat_out[bank] = 0x00;
	ret |= mc5587_write(dev->client, GPIODIR2, dev->dir[bank]);
	ret |= mc5587_write(dev->client, GPODAT2, dev->dat_out[bank]);

	bank = 2;
	bit = 0x03;	/* 0b00000011 */
	dev->dir[bank] = bit;
	dev->dat_out[bank] = 0x00;
	ret |= mc5587_write(dev->client, GPIODIR3, dev->dir[bank]);
	ret |= mc5587_write(dev->client, GPODAT3, dev->dat_out[bank]);

	mc5587_read(dev->client, GPINTST1);	/* clear */
	mc5587_read(dev->client, GPINTST2);
	mc5587_read(dev->client, GPINTST3);

	pr_info("[%s] GPIO Expander Init setting\n", __func__);

	return ret;
}

#ifdef CONFIG_OF
static int mc5587_parse_dt(struct device *dev,
		struct mc5587_gpio_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int ret;

	ret = of_property_read_u32(np, "mc5587,gpio_start", &pdata->gpio_start);
	if (ret < 0) {
		pr_err("[%s]: Unable to read mc5587,gpio_start\n", __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "mc5587,ngpio", &pdata->ngpio);
	if (ret < 0) {
		pr_err("[%s]: Unable to read mc5587,ngpio\n", __func__);
		return ret;
	}
	ret = of_property_read_u32(np, "mc5587,irq-base", &pdata->irq_base);
	if (ret < 0) {
		pr_err("[%s]: Unable to read mc5587,irq-base\n", __func__);
		return ret;
	}
	pdata->reset_gpio = of_get_named_gpio(np, "mc5587,reset-gpio", 0);
	pdata->irq_gpio = of_get_named_gpio(np, "mc5587,irq-gpio", 0);

	dev->platform_data = pdata;
	pr_info("[%s] gpio_start=[%d] ngpio=[%d] reset_gpio=[%d] irq_gpio=[%d]\n",
			__func__, pdata->gpio_start, pdata->ngpio,
			pdata->reset_gpio, pdata->irq_gpio);
	return 0;
}
#endif

static void mc5587_power_ctrl(bool enable)
{
	int ret = 0;
	static struct regulator *reg_s4;

	if (!reg_s4) {
		reg_s4 = regulator_get(NULL, "8084_s4");
		if (IS_ERR(reg_s4)) {
			pr_err("%s: could not get 8917_s4, rc = %ld\n",
					__func__, PTR_ERR(reg_s4));
			return;
		}
		ret = regulator_set_voltage(reg_s4, 1800000, 1800000);
		if (ret) {
			pr_err("%s: unable to set s4 voltage to 1.8V\n",
					__func__);
			return;
		}
	}

	if (enable) {
		if (regulator_is_enabled(reg_s4))
			pr_err("%s: S4(1.8V) is enabled\n", __func__);
		else
			ret = regulator_enable(reg_s4);
		if (ret) {
			pr_err("%s: enable s4 failed, rc=%d\n",
					__func__, ret);
			return;
		}
		pr_info("%s: gpio expander 1.8V on is finished.\n", __func__);
	} else {
		if (regulator_is_enabled(reg_s4))
			ret = regulator_disable(reg_s4);
		else
			pr_err("%s: S4(1.8V) is disabled\n", __func__);
		if (ret) {
			pr_err("%s: disable S4 failed, rc=%d\n",
					__func__, ret);
			return;
		}
		pr_info("%s: gpio expander 1.8V off is finished.\n", __func__);
	}
	pr_err("[mc5587 gpio expander] %s enable(%d)\n", __func__, enable);
	return;
}

static int mc5587_reset_chip(struct mc5587_gpio_platform_data *pdata)
{
	int retval;
	int reset_mc5587 = pdata->reset_gpio;

	if (gpio_is_valid(reset_mc5587)) {
		retval = gpio_request(reset_mc5587,
				"mc5587_reset_gpio");
		if (retval) {
			pr_err("%s: unable to request gpio [%d]\n",
					__func__, reset_mc5587);
			return retval;
		}

		retval = gpio_direction_output(reset_mc5587, 1);
		if (retval) {
			pr_err("%s: unable to set direction for gpio [%d]\n",
					__func__, reset_mc5587);
			return retval;
		}

		usleep(100);
		gpio_set_value(reset_mc5587, 0);
		usleep(100);
		gpio_set_value(reset_mc5587, 1);
		pr_info("%s: gpio expander reset.\n", __func__);

		gpio_free(reset_mc5587);
		return 0;
	} else 
		pr_err("%s: reset_mc5587 fail\n", __func__);
	return 0;
}

struct device *gpio_dev;
extern struct class *sec_class;

static ssize_t store_mc5587_gpio_inout(struct device *dev,
		struct device_attribute *devattr,
		const char *buf, size_t count)
{
	int retval, off, val, gpio_mc5587;
	char in_out, msg[13];
	struct mc5587_gpio *data = dev_get_drvdata(dev);                 

	retval = sscanf(buf, "%c %d %d", &in_out, &off, &val);
	if (retval == 0) {
		dev_err(&data->client->dev, "[%s] fail to mc5587 out.\n", __func__);
		return count;
	}    
	
	gpio_mc5587 = 300 + off;
	sprintf(msg, "exp-gpio%d\n", off);
	if (gpio_is_valid(gpio_mc5587)) {
		retval = gpio_request(gpio_mc5587, msg);
		if (retval) {
			pr_err("[%s] unable to request gpio=[%d] err=[%d]\n",
					__func__, gpio_mc5587, retval);
			return count;
		}

		if (in_out == 'i') {
			retval = gpio_direction_input(gpio_mc5587);
			val = gpio_get_value(gpio_mc5587);
		}
		else 
			retval = gpio_direction_output(gpio_mc5587, val);

		if (retval) 
			pr_err("%s: unable to set direction for gpio [%d]\n",
						__func__, gpio_mc5587);

		gpio_free(gpio_mc5587);
	} 

	pr_info("mc5587 mode set to dir[%c], offset[%d], val[%d]\n", in_out, off, val);  

	return count;
}

static DEVICE_ATTR(expgpio, 0664, NULL, store_mc5587_gpio_inout);

static int __devinit mc5587_gpio_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct device_node *np = client->dev.of_node;
	struct mc5587_gpio_platform_data *pdata = NULL;
	struct mc5587_gpio *dev;
	struct gpio_chip *gc;
	int ret, i, revid, retval;

	if (!i2c_check_functionality(client->adapter,
				I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "SMBUS Byte Data not Supported\n");
		return -EIO;
	}
#ifdef CONFIG_OF
	if (np) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct mc5587_gpio_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}

		retval = mc5587_parse_dt(&client->dev, pdata);
		if (retval) {
			pr_err("[%s] mc5587 parse dt failed\n", __func__);
			return retval;
		}

		mc5587_power_ctrl(true);
		mc5587_reset_chip(pdata);
	} else {
		pdata = client->dev.platform_data;
		pr_info("GPIO Expender failed to align dtsi %s",
				__func__);
	}
#else
	pdata = client->dev.platform_data;
#endif

	if (pdata == NULL) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&client->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	dev->client = client;

	gc = &dev->gpio_chip;
	gc->direction_input = mc5587_gpio_direction_input;
	gc->direction_output = mc5587_gpio_direction_output;
	gc->get = mc5587_gpio_get_value;
	gc->set = mc5587_gpio_set_value;
	gc->request = mc5587_gpio_request;
	gc->free = mc5587_gpio_free;
	gc->can_sleep = 0;
	/*Require dev to use the of_gpio api*/
	gc->dev = &client->dev;
#ifdef CONFIG_GPIO_MC5587_IRQ
	gc->to_irq = mc5587_gpio_to_irq;
#endif
	gc->base = pdata->gpio_start;
	gc->ngpio = pdata->ngpio;
	gc->label = client->name;
	gc->owner = THIS_MODULE;

	mutex_init(&dev->lock);

	ret = mc5587_read(dev->client, DEVID);
	if (ret < 0)
		goto err;

	revid = ret & MC5587_DEVICE_ID_MASK;

	for (i = 0, ret = 0; i <= MC5587_BANK(MC5587_MAXGPIO); i++) {
		dev->dat_out[i] = mc5587_read(client, GPODAT1 + i);
		dev->dir[i] = mc5587_read(client, GPIODIR1 + i);
		ret |= mc5587_write(client, KPMSEL1 + i, 0);
		ret |= mc5587_write(client, GPIPU1 + i,
				(pdata->pullup_dis_mask >> (8 * i)) & 0xFF);
		ret |= mc5587_write(client, GPINTEN1 + i, 0);
		if (ret)
			goto err;
	}

	mc5587_gpio_setup(dev);

	if (pdata->irq_base) {
		if (WA_DELAYED_READOUT_REVID(revid)) {
			dev_warn(&client->dev, "GPIO int not supported\n");
		} else {
			ret = mc5587_irq_setup(dev);
			if (ret)
				goto err;
		}
	}

	ret = gpiochip_add(&dev->gpio_chip);
	if (ret)
		goto err_irq;

	dev_info(&client->dev, "gpios %d..%d (IRQ Base %d) on a %s Rev. %d\n",
			gc->base, gc->base + gc->ngpio - 1,
			pdata->irq_base, client->name, revid);

	if (pdata->setup) {
		ret = pdata->setup(client, gc->base, gc->ngpio, pdata->context);
		if (ret < 0)
			dev_warn(&client->dev, "setup failed, %d\n", ret);
	} else {
		pr_info("[%s] No pdata->setup\n", __func__);
	}

	gpio_dev = device_create(sec_class, NULL, 0, dev, "expander");
	if (IS_ERR(gpio_dev)) {
		dev_err(&client->dev,
				"Failed to create device for expander\n");
		ret = -ENODEV;
		goto err;
	}

	ret = sysfs_create_file(&gpio_dev->kobj, &dev_attr_expgpio.attr);
	if (ret) {
		dev_err(&client->dev,
				"Failed to create sysfs group for expander\n");
		goto err_destroy;
	}

	i2c_set_clientdata(client, dev);

	return 0;

err_irq:
	mc5587_irq_teardown(dev);
err_destroy:                          
	device_destroy(sec_class, 0); 
err:
	kfree(dev);
	return ret;
}

static int __devexit mc5587_gpio_remove(struct i2c_client *client)
{
	struct mc5587_gpio_platform_data *pdata = client->dev.platform_data;
	struct mc5587_gpio *dev = i2c_get_clientdata(client);
	int ret;

	if (pdata->teardown) {
		ret = pdata->teardown(client,
				      dev->gpio_chip.base, dev->gpio_chip.ngpio,
				      pdata->context);
		if (ret < 0) {
			dev_err(&client->dev, "teardown failed %d\n", ret);
			return ret;
		}
	}

	if (dev->irq_base)
		free_irq(dev->client->irq, dev);

	ret = gpiochip_remove(&dev->gpio_chip);
	if (ret) {
		dev_err(&client->dev, "gpiochip_remove failed %d\n", ret);
		return ret;
	}

	kfree(dev);
	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id mc5587_dt_ids[] = {
	{ .compatible = "mc5587,gpio-expander",},
};
#endif
static const struct i2c_device_id mc5587_gpio_id[] = {
	{DRV_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, mc5587_gpio_id);

static struct i2c_driver mc5587_gpio_driver = {
	.driver = {
		   .name = DRV_NAME,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(mc5587_dt_ids),
#endif
		   },
	.probe = mc5587_gpio_probe,
	.remove = __devexit_p(mc5587_gpio_remove),
	.id_table = mc5587_gpio_id,
};

static int __init mc5587_gpio_init(void)
{
	return i2c_add_driver(&mc5587_gpio_driver);
}

module_init(mc5587_gpio_init);

static void __exit mc5587_gpio_exit(void)
{
	i2c_del_driver(&mc5587_gpio_driver);
}

module_exit(mc5587_gpio_exit);

MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
MODULE_DESCRIPTION("GPIO MC5587 Driver");
MODULE_LICENSE("GPL");
