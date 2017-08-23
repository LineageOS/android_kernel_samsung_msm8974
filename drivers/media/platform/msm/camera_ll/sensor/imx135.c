
/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "msm_sensor.h"
#define IMX135_SENSOR_NAME "imx135"

#define IMX135_DEBUG

#undef CDBG
#ifdef IMX135_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif
DEFINE_MSM_MUTEX(imx135_mut);


static struct msm_sensor_ctrl_t imx135_s_ctrl;

struct class *camera_class;


static struct v4l2_subdev_info imx135_subdev_info[] = {
	{
		.code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt = 1,
		.order = 0,
	},
};

static const struct i2c_device_id imx135_i2c_id[] = {
	{IMX135_SENSOR_NAME, (kernel_ulong_t)&imx135_s_ctrl},
	{ }
};

static int32_t msm_imx135_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &imx135_s_ctrl);
}
static struct i2c_driver imx135_i2c_driver = {
	.id_table = imx135_i2c_id,
	.probe  = msm_imx135_i2c_probe,
	.driver = {
		.name = IMX135_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx135_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx135_dt_match[] = {
	{.compatible = "qcom,imx135", .data = &imx135_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, imx135_dt_match);

static struct platform_driver imx135_platform_driver = {
	.driver = {
		.name = "qcom,imx135",
		.owner = THIS_MODULE,
		.of_match_table = imx135_dt_match,
	},
};

extern uint16_t back_cam_fw_version;

static ssize_t back_camera_type_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char type[] = "SONY_IMX135_FIMC_IS\n";

	 return snprintf(buf, sizeof(type), "%s", type);
}

static ssize_t front_camera_type_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char cam_type[] = "S5K6B2YX\n";

	 return snprintf(buf, sizeof(cam_type), "%s", cam_type);
}


static ssize_t back_camera_firmware_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_MACH_KS01SKT) || defined(CONFIG_MACH_KS01KTT)\
	|| defined(CONFIG_MACH_KS01LGT)
	char cam_fw[] = "O13Q0SAGC01 O13Q0SAGC01\n";/*Camsys_module,13mega_pixel,Qualcomm_isp,Sony_sensor*/
#else
	char cam_fw[] = "D13QSGF01OA D13QSGF01OA\n";/*Camsys_module,13mega_pixel,Qualcomm_isp,Sony_sensor*/
#endif

	return snprintf(buf, sizeof(cam_fw), "%s", cam_fw);

}

static ssize_t front_camera_firmware_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char cam_fw[] = "S5K6B2YX S5K6B2YX\n";

	return  snprintf(buf, sizeof(cam_fw), "%s", cam_fw);
}

static DEVICE_ATTR(rear_camtype, S_IRUGO, back_camera_type_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO, back_camera_firmware_show, NULL);
static DEVICE_ATTR(front_camtype, S_IRUGO, front_camera_type_show, NULL);
static DEVICE_ATTR(front_camfw, S_IRUGO, front_camera_firmware_show, NULL);

static int32_t imx135_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	struct device 	*cam_dev_back;
	struct device 	*cam_dev_front;
	match = of_match_device(imx135_dt_match, &pdev->dev);
	camera_class = class_create(THIS_MODULE, "camera");
	if (IS_ERR(camera_class))
	    pr_err("failed to create device cam_dev_rear!\n");

	rc = msm_sensor_platform_probe(pdev, match->data);
	printk("%s00:%d\n", __func__, __LINE__);

	printk("%s01:%d\n", __func__, __LINE__);

	cam_dev_back = device_create(camera_class, NULL,
		1, NULL, "rear");
	if (IS_ERR(cam_dev_back)) {
		printk("Failed to create cam_dev_back device!\n");
	}

	if (device_create_file(cam_dev_back, &dev_attr_rear_camtype) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_rear_camtype.attr.name);
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_camfw) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_rear_camfw.attr.name);
	}

	cam_dev_front = device_create(camera_class, NULL,
		2, NULL, "front");
	if (IS_ERR(cam_dev_front)) {
		printk("Failed to create cam_dev_front device!");
	}

	if (device_create_file(cam_dev_front, &dev_attr_front_camtype) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_front_camtype.attr.name);
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_camfw) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_front_camfw.attr.name);
	}
	return rc;
}

static int __init imx135_init_module(void)
{
	int32_t rc = 0;
	CDBG("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&imx135_platform_driver,
		imx135_platform_probe);
	if (!rc) {
		pr_info("%s: probe success\n", __func__);
		return rc;
	}
	CDBG("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&imx135_i2c_driver);
}

static void __exit imx135_exit_module(void)
{
	CDBG("%s:%d\n", __func__, __LINE__);
	if (imx135_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx135_s_ctrl);
		platform_driver_unregister(&imx135_platform_driver);
	} else
		i2c_del_driver(&imx135_i2c_driver);
	return;
}

static struct msm_sensor_ctrl_t imx135_s_ctrl = {
	.sensor_i2c_client = &imx135_sensor_i2c_client,
	.msm_sensor_mutex = &imx135_mut,
	.sensor_v4l2_subdev_info = imx135_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx135_subdev_info),
};

module_init(imx135_init_module);
module_exit(imx135_exit_module);
MODULE_DESCRIPTION("imx135");
MODULE_LICENSE("GPL v2");
