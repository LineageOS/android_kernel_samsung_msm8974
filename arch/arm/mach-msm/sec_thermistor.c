/* sec_thermistor.c
 *
 * Copyright (C) 2011 Samsung Electronics
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
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <mach/sec_thermistor.h>

#include <linux/qpnp/qpnp-adc.h>

#define ADC_SAMPLING_CNT	7

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
//#ifndef SSRM_TEST
//#define SSRM_TEST
//#endif
#endif

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM)
#if defined(CONFIG_ARCH_MSM8974PRO)
#define FLASH_THERM_CH	LR_MUX9_PU1_AMUX_THM5
#else
#define FLASH_THERM_CH	LR_MUX9_PU2_AMUX_THM5
#endif
#elif defined(CONFIG_MACH_KLIMT_LTE_DCM)
#define FLASH_THERM_CH	LR_MUX5_PU2_AMUX_THM2
#endif

#if defined(CONFIG_ARCH_MSM8974PRO)
#define MSM_THERM_CH	LR_MUX4_PU1_AMUX_THM1
#else
#define MSM_THERM_CH	LR_MUX4_PU2_AMUX_THM1
#endif

struct sec_therm_info {
	struct device *dev;
	struct sec_therm_platform_data *pdata;
	struct delayed_work polling_work;

	int curr_temperature;
	int curr_temp_adc;
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM)|| defined(CONFIG_MACH_KLIMT_LTE_DCM)
	int curr_temperature_flash_led;
	int curr_temp_adc_flash_led;
#endif
};

static int sec_therm_get_adc_data(struct sec_therm_info *info);
static int convert_adc_to_temper(struct sec_therm_info *info, unsigned int adc);

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
static int sec_therm_get_adc_data_flash_led(struct sec_therm_info *info);
#endif

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)
static int convert_adc_flash_to_temper(struct sec_therm_info *info, unsigned int adc);
#endif

#if defined (SSRM_TEST)
static int tempTest;
#endif

static ssize_t sec_therm_show_temperature(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int adc;
	int temper;

	struct sec_therm_info *info = dev_get_drvdata(dev);

	adc = sec_therm_get_adc_data(info);
	temper = convert_adc_to_temper(info, adc);

        #if defined (SSRM_TEST)
        temper = tempTest;
        #endif

	return sprintf(buf, "%d\n", temper);
}

#if defined (SSRM_TEST)
static ssize_t sec_therm_store_temperature(struct device *dev,
				   struct device_attribute *attr, const char *buf, size_t count)
{
	sscanf(buf, "%d", &tempTest);
	return count;
}
#endif

static ssize_t sec_therm_show_temp_adc(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int adc;
	struct sec_therm_info *info = dev_get_drvdata(dev);

	adc = sec_therm_get_adc_data(info);

	return sprintf(buf, "%d\n", adc);
}

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
static ssize_t sec_therm_show_temperature_flash_led(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int adc;
	int temper;

	struct sec_therm_info *info = dev_get_drvdata(dev);

	adc = sec_therm_get_adc_data_flash_led(info);
	#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)
	temper = convert_adc_flash_to_temper(info, adc);
	#else
	temper = convert_adc_to_temper(info, adc);
	#endif

	dev_info(info->dev, "%s: adc_flash=%d\n", __func__, adc);
	return sprintf(buf, "%d\n", temper);
}

static ssize_t sec_therm_show_temp_adc_flash_led(struct device *dev,
				   struct device_attribute *attr,
				   char *buf)
{
	int adc;
	struct sec_therm_info *info = dev_get_drvdata(dev);

	adc = sec_therm_get_adc_data_flash_led(info);

	return sprintf(buf, "%d\n", adc);
}
#endif

#if defined (SSRM_TEST)
static DEVICE_ATTR(temperature, S_IRUGO | S_IWUSR, sec_therm_show_temperature, sec_therm_store_temperature);
#else
static DEVICE_ATTR(temperature, S_IRUGO, sec_therm_show_temperature, NULL);
#endif
static DEVICE_ATTR(temp_adc, S_IRUGO, sec_therm_show_temp_adc, NULL);

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
static DEVICE_ATTR(temperature_flash, S_IRUGO, sec_therm_show_temperature_flash_led, NULL);
static DEVICE_ATTR(temp_adc_flash, S_IRUGO, sec_therm_show_temp_adc_flash_led, NULL);
#endif

static struct attribute *sec_therm_attributes[] = {
	&dev_attr_temperature.attr,
	&dev_attr_temp_adc.attr,
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
	&dev_attr_temperature_flash.attr,
	&dev_attr_temp_adc_flash.attr,
#endif
	NULL
};

static const struct attribute_group sec_therm_group = {
	.attrs = sec_therm_attributes,
};

static int sec_therm_get_adc_data(struct sec_therm_info *info)
{
	int rc = 0;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i, adc_data;

	struct qpnp_vadc_result results;

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {

		rc = qpnp_vadc_read(NULL, MSM_THERM_CH , &results);

		if (rc) {
			pr_err("error reading AMUX %d, rc = %d\n",
						MSM_THERM_CH, rc);
			goto err;
		}
		adc_data = results.adc_code;

		if (i == 0) {
			pr_err("reading MSM_THERM_CH [rc = %d] [adc_code = %d]\n",
									rc,results.adc_code);
		}

		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}

		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (ADC_SAMPLING_CNT - 2);

err:
	return rc;

}

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
static int sec_therm_get_adc_data_flash_led(struct sec_therm_info *info)
{
	int rc = 0;
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i, adc_data;

	struct qpnp_vadc_result results;

	for (i = 0; i < ADC_SAMPLING_CNT; i++) {

		rc = qpnp_vadc_read(NULL, FLASH_THERM_CH, &results);

		if (rc) {
			pr_err("error reading AMUX %d, rc = %d\n",
						LR_MUX9_PU2_AMUX_THM5, rc);
			goto err;
		}
		adc_data = results.adc_code;

		pr_err("reading LR_MUX9_PU2_AMUX_THM5 [rc = %d] [adc_code = %d]\n",
									rc,results.adc_code);
		if (i != 0) {
			if (adc_data > adc_max)
				adc_max = adc_data;
			else if (adc_data < adc_min)
				adc_min = adc_data;
		} else {
			adc_max = adc_data;
			adc_min = adc_data;
		}

		adc_total += adc_data;
	}

	return (adc_total - adc_max - adc_min) / (ADC_SAMPLING_CNT - 2);

err:
	return rc;

}

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)
static int convert_adc_flash_to_temper(struct sec_therm_info *info, unsigned int adc)
{
	int low = 0;
	int high = 0;
	int mid = 0;
	int temp = 0;
	int temp2 = 0;

	if (!info->pdata->adc_table_flash || !info->pdata->adc_flash_arr_size) {
		/* using fake temp */
		return 300;
	}

	high = info->pdata->adc_flash_arr_size - 1;

	if (info->pdata->adc_table_flash[low].adc >= adc) {
		temp = info->pdata->adc_table_flash[low].temperature;
		goto convert_adc_to_temp_goto;
	} else if (info->pdata->adc_table_flash[high].adc <= adc) {
		temp = info->pdata->adc_table_flash[high].temperature;
		goto convert_adc_to_temp_goto;
	}

	while (low <= high) {
		mid = (low + high) / 2;
		if (info->pdata->adc_table_flash[mid].adc > adc) {
			high = mid - 1;
		} else if (info->pdata->adc_table_flash[mid].adc < adc) {
			low = mid + 1;
		} else {
			temp = info->pdata->adc_table_flash[mid].temperature;
			goto convert_adc_to_temp_goto;
		}
	}

	temp = info->pdata->adc_table_flash[high].temperature;

	temp2 = (info->pdata->adc_table_flash[low].temperature -
			info->pdata->adc_table_flash[high].temperature) *
			(adc - info->pdata->adc_table_flash[high].adc);

	temp += temp2 /
		(info->pdata->adc_table_flash[low].adc -
			info->pdata->adc_table_flash[high].adc);

convert_adc_to_temp_goto:

	return temp;
}
#endif
#endif

static int convert_adc_to_temper(struct sec_therm_info *info, unsigned int adc)
{
	int low = 0;
	int high = 0;
	int mid = 0;
	int temp = 0;
	int temp2 = 0;

	if (!info->pdata->adc_table || !info->pdata->adc_arr_size) {
		/* using fake temp */
		return 300;
	}

	high = info->pdata->adc_arr_size - 1;

	if (info->pdata->adc_table[low].adc >= adc) {
		temp = info->pdata->adc_table[low].temperature;
		goto convert_adc_to_temp_goto;
	} else if (info->pdata->adc_table[high].adc <= adc) {
		temp = info->pdata->adc_table[high].temperature;
		goto convert_adc_to_temp_goto;
	}

	while (low <= high) {
		mid = (low + high) / 2;
		if (info->pdata->adc_table[mid].adc > adc) {
			high = mid - 1;
		} else if (info->pdata->adc_table[mid].adc < adc) {
			low = mid + 1;
		} else {
			temp = info->pdata->adc_table[mid].temperature;
			goto convert_adc_to_temp_goto;
		}
	}

	temp = info->pdata->adc_table[high].temperature;

	temp2 = (info->pdata->adc_table[low].temperature -
			info->pdata->adc_table[high].temperature) *
			(adc - info->pdata->adc_table[high].adc);

	temp += temp2 /
		(info->pdata->adc_table[low].adc -
			info->pdata->adc_table[high].adc);

convert_adc_to_temp_goto:

	return temp;
}

static void notify_change_of_temperature(struct sec_therm_info *info)
{
	char temp_buf[20];
	char siop_buf[20];
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
	char *envp[4];
#else
	char *envp[3];
#endif
	int env_offset = 0;
	int siop_level = -1;
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
	char temp_buf_flash[20];


	snprintf(temp_buf, sizeof(temp_buf), "SUBTEMPERATURE=%d",
		 info->curr_temperature);
	envp[env_offset++] = temp_buf;

	snprintf(temp_buf_flash, sizeof(temp_buf_flash), "FLASH_TEMP=%d",
		 info->curr_temperature_flash_led);
	envp[env_offset++] = temp_buf_flash;
#else
	snprintf(temp_buf, sizeof(temp_buf), "TEMPERATURE=%d",
		 info->curr_temperature);
	envp[env_offset++] = temp_buf;
#endif

	if (info->pdata->get_siop_level)
		siop_level =
		    info->pdata->get_siop_level(info->curr_temperature);
	if (siop_level >= 0) {
		snprintf(siop_buf, sizeof(siop_buf), "SIOP_LEVEL=%d",
			 siop_level);
		envp[env_offset++] = siop_buf;
		dev_info(info->dev, "%s: uevent: %s\n", __func__, siop_buf);
	} else {
		envp[env_offset++] = NULL;
	}

	envp[env_offset] = NULL;

	dev_info(info->dev, "%s: siop_level=%d\n", __func__, siop_level);
	dev_info(info->dev, "%s: uevent: %s\n", __func__, temp_buf);
	kobject_uevent_env(&info->dev->kobj, KOBJ_CHANGE, envp);
}

static void sec_therm_polling_work(struct work_struct *work)
{
	struct sec_therm_info *info =
		container_of(work, struct sec_therm_info, polling_work.work);
	int adc;
	int temper;
#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
	int temper_flash;
	int adc_flash;
#endif

	adc = sec_therm_get_adc_data(info);
	dev_info(info->dev, "%s: adc=%d\n", __func__, adc);

	if (adc < 0)
		goto out;

	temper = convert_adc_to_temper(info, adc);
	dev_info(info->dev, "%s: temper=%d\n", __func__, temper);

#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI) || \
	defined(CONFIG_MACH_JS01LTEDCM) || defined(CONFIG_MACH_KLTE_JPN) || \
	defined(CONFIG_MACH_KACTIVELTE_DCM) || defined(CONFIG_MACH_KLIMT_LTE_DCM)
	adc_flash = sec_therm_get_adc_data_flash_led(info);
	dev_info(info->dev, "%s: adc_flash=%d\n", __func__, adc_flash);

	if (adc_flash < 0)
		goto out;

	#if defined(CONFIG_MACH_HLTEDCM) || defined(CONFIG_MACH_HLTEKDI)
	temper_flash= convert_adc_flash_to_temper(info, adc_flash);
	#else
	temper_flash= convert_adc_to_temper(info, adc_flash);
	#endif
	dev_info(info->dev, "%s: temper_flash=%d\n", __func__, temper_flash);

	/* if temperature was changed, notify to framework */
	if (info->curr_temperature != temper || info->curr_temperature_flash_led!= temper_flash) {
		info->curr_temp_adc = adc;
		info->curr_temperature = temper;

		info->curr_temp_adc_flash_led = adc_flash;
		info->curr_temperature_flash_led = temper_flash;
		notify_change_of_temperature(info);
	}
#else
	/* if temperature was changed, notify to framework */
	if (info->curr_temperature != temper) {
		info->curr_temp_adc = adc;
		info->curr_temperature = temper;
		notify_change_of_temperature(info);
	}
#endif

out:
	schedule_delayed_work(&info->polling_work,
			msecs_to_jiffies(info->pdata->polling_interval));
}

static __devinit int sec_therm_probe(struct platform_device *pdev)
{
	struct sec_therm_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct sec_therm_info *info;
	int ret = 0;

	dev_info(&pdev->dev, "%s: SEC Thermistor Driver Loading\n", __func__);

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	platform_set_drvdata(pdev, info);

	info->dev = &pdev->dev;
	info->pdata = pdata;

	ret = sysfs_create_group(&info->dev->kobj, &sec_therm_group);

	if (ret) {
		dev_err(info->dev,
			"failed to create sysfs attribute group\n");

		kfree(info);
	}

	if (!(pdata->no_polling)) {

		INIT_DELAYED_WORK_DEFERRABLE(&info->polling_work,
			sec_therm_polling_work);
		schedule_delayed_work(&info->polling_work,
			msecs_to_jiffies(info->pdata->polling_interval));

	}
	return ret;
}

static int __devexit sec_therm_remove(struct platform_device *pdev)
{
	struct sec_therm_info *info = platform_get_drvdata(pdev);

	if (!info)
		return 0;

	sysfs_remove_group(&info->dev->kobj, &sec_therm_group);

	if (!(info->pdata->no_polling))
		cancel_delayed_work(&info->polling_work);
	kfree(info);

	return 0;
}

#ifdef CONFIG_PM
static int sec_therm_suspend(struct device *dev)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);

	if (!(info->pdata->no_polling))
		cancel_delayed_work(&info->polling_work);

	return 0;
}

static int sec_therm_resume(struct device *dev)
{
	struct sec_therm_info *info = dev_get_drvdata(dev);

	if (!(info->pdata->no_polling))
		schedule_delayed_work(&info->polling_work,
			msecs_to_jiffies(info->pdata->polling_interval));
	return 0;
}
#else
#define sec_therm_suspend	NULL
#define sec_therm_resume	NULL
#endif /* CONFIG_PM */

static const struct dev_pm_ops sec_thermistor_pm_ops = {
	.suspend = sec_therm_suspend,
	.resume = sec_therm_resume,
};

static struct platform_driver sec_thermistor_driver = {
	.driver = {
		   .name = "sec-thermistor",
		   .owner = THIS_MODULE,
		   .pm = &sec_thermistor_pm_ops,
	},
	.probe = sec_therm_probe,
	.remove = __devexit_p(sec_therm_remove),
};

static int __init sec_therm_init(void)
{
	pr_info("func:%s\n", __func__);
	#if defined (SSRM_TEST)
	tempTest=333;
	#endif
	return platform_driver_register(&sec_thermistor_driver);
}
module_init(sec_therm_init);

static void __exit sec_therm_exit(void)
{
	platform_driver_unregister(&sec_thermistor_driver);
}
module_exit(sec_therm_exit);

MODULE_AUTHOR("ms925.kim@samsung.com");
MODULE_DESCRIPTION("sec thermistor driver");
MODULE_LICENSE("GPL");
