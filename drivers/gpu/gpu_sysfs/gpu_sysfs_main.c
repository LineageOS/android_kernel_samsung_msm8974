#include	"gpu_sysfs_header.h"

/* Platform device global pointer. */
static struct platform_device *gpu_sysfs_pdev;

/* *
 * *********************************************************************
 * Device attribute macros, linking functions with sysfs.
 * *********************************************************************
 * */
DEVICE_ATTR(gpu_min_clock, 0444, gpu_min_clock_show, NULL);
DEVICE_ATTR(gpu_max_clock, 0444, gpu_max_clock_show, NULL);
DEVICE_ATTR(gpu_busy, 0444, gpu_busy_show, NULL);
DEVICE_ATTR(gpu_voltage, 0444, gpu_vol_show, NULL);
DEVICE_ATTR(gpu_clock, 0666, gpu_freq_show, gpu_freq_write);
DEVICE_ATTR(gpu_freq_table, 0444, gpu_freq_table_show, NULL);
DEVICE_ATTR(gpu_governor, 0666, gpu_governor_show, gpu_governor_write);
DEVICE_ATTR(gpu_available_governor, 0444, gpu_available_governor_show, NULL);
DEVICE_ATTR(gpu_cores_config, 0444, gpu_cores_config_show, NULL);
DEVICE_ATTR(gpu_tmu, 0444, gpu_tmu_show, NULL);
DEVICE_ATTR(gpu_model, 0444, gpu_model_show, NULL);
DEVICE_ATTR(gpu_version, 0444, gpu_version_show, NULL);
DEVICE_ATTR(gpu_mem, 0444, gpu_mem_show, NULL);
DEVICE_ATTR(fps, 0666, fps_show, fps_write);

/* The below function will generate sysfs during initialization stage. */
int gpu_sysfs_create_sysfs_files(struct device *dev)
{
	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	if (device_create_file(dev, &dev_attr_gpu_min_clock))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_max_clock))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_busy))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_voltage))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_clock))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_freq_table))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_governor))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_available_governor))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_cores_config))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_tmu))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_model))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_version))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_gpu_mem))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	if (device_create_file(dev, &dev_attr_fps))
	{
		pr_info("GPU_SYSFS: Couldn't create sysfs file %d\n", __LINE__);
		goto out;
	}

	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);
	return 0;

out:
	return -ENOENT;
}

void gpu_sysfs_remove_sysfs_files(struct device *dev)
{
	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	device_remove_file(dev, &dev_attr_gpu_min_clock);
	device_remove_file(dev, &dev_attr_gpu_max_clock);
	device_remove_file(dev, &dev_attr_gpu_busy);
	device_remove_file(dev, &dev_attr_gpu_voltage);
	device_remove_file(dev, &dev_attr_gpu_clock);
	device_remove_file(dev, &dev_attr_gpu_freq_table);
	device_remove_file(dev, &dev_attr_gpu_governor);
	device_remove_file(dev, &dev_attr_gpu_available_governor);
	device_remove_file(dev, &dev_attr_gpu_cores_config);
	device_remove_file(dev, &dev_attr_gpu_tmu);
	device_remove_file(dev, &dev_attr_gpu_model);
	device_remove_file(dev, &dev_attr_gpu_version);
	device_remove_file(dev, &dev_attr_gpu_mem);
	device_remove_file(dev, &dev_attr_fps);
}

sruk_bool gpu_sysfs_device_init(sruk_device * const kbdev)
{
	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	if (gpu_sysfs_create_sysfs_files(kbdev->osdev.dev))
		return SRUK_FALSE;

	return SRUK_TRUE;
}

static int gpu_sysfs_probe(struct platform_device *pdev)
{
	struct sruk_device *kbdev;
	struct sruk_os_device *osdev;
	int err;

	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	kbdev = kzalloc(sizeof(sruk_device), GFP_KERNEL);
	if (!kbdev)
	{
		dev_err(&pdev->dev, "GPU_SYSFS: Can't allocate device\n");
		err = -ENOMEM;
		goto out;
	}

	osdev = &kbdev->osdev;
	osdev->dev = &pdev->dev;

	if (SRUK_TRUE != gpu_sysfs_device_init(kbdev)) {
		pr_info( "GPU_SYSFS: gpu_sysfs_device_init failed");
		dev_err(&pdev->dev, "GPU_SYSFS: Can't initialize device\n");
		err = -ENOMEM;
		goto out_reg_unmap;
	}

	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);
	osdev = NULL;
	kzfree(kbdev);
	return 0;

out_reg_unmap:
	osdev = NULL;
	kzfree(kbdev);
out:
	return err;
}


static int gpu_sysfs_remove(struct platform_device *pdev)
{
	struct sruk_device *kbdev = (struct sruk_device *)(&pdev->dev);

	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	if (!kbdev)
		return -ENODEV;

	gpu_sysfs_remove_sysfs_files(kbdev->osdev.dev);

	kzfree(kbdev);

	return 0;
}


/* Platform driver operations and assigning function pointers. */
static struct platform_driver gpu_sysfs_platform_driver =
{
    .probe = gpu_sysfs_probe,
    .remove = gpu_sysfs_remove,
    .driver =
    {
        .name =		GPU_SYSFS_MODULE_NAME,
        .owner =	THIS_MODULE,
    },
};

static int __init gpu_sysfs_init(void)
{
	int ret;

	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	gpu_sysfs_pdev = platform_device_alloc(GPU_SYSFS_MODULE_NAME, -1);
	if (!gpu_sysfs_pdev)
	{
		pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);
		pr_err("Failed to allocate dummy regulator device\n");
		return 0;
	}

	/* Add platform device. */
	ret = platform_device_add(gpu_sysfs_pdev);
	if (ret != 0)
	{
		pr_err("GPU_SYSFS: Failed to register dummy regulator device: %d\n", ret);
		platform_device_put(gpu_sysfs_pdev);
		return 0;
	}

	ret = platform_driver_register(&gpu_sysfs_platform_driver);
	if (ret != 0)
	{
		pr_err("GPU_SYSFS: Failed to register dummy regulator driver: %d\n", ret);
		platform_device_unregister(gpu_sysfs_pdev);
	}

	pr_info("GPU_SYSFS ----------- %s -- %d", __FUNCTION__, __LINE__);

	return 0;
}

static void __exit gpu_sysfs_exit(void)
{
	pr_info( "GPU_SYSFS: Unified Sysfs for GPU: Exiting" );
	platform_driver_unregister(&gpu_sysfs_platform_driver);
}


module_init(gpu_sysfs_init);
module_exit(gpu_sysfs_exit);

MODULE_AUTHOR("SRUK_GFX");
MODULE_DESCRIPTION("GPU_SYSFS: Module for Unified Sysfs for GPU");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
