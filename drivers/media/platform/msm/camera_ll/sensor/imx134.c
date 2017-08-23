
/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
#define IMX134_SENSOR_NAME "imx134"

#undef CDBG
#ifdef IMX135_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif

DEFINE_MSM_MUTEX(imx134_mut);

struct class *camera_class;

static struct msm_sensor_ctrl_t imx134_s_ctrl;

static struct v4l2_subdev_info imx134_subdev_info[] = {
	{
		.code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt = 1,
		.order = 0,
	},
};

static const struct i2c_device_id imx134_i2c_id[] = {
	{IMX134_SENSOR_NAME, (kernel_ulong_t)&imx134_s_ctrl},
	{ }
};

static int32_t msm_imx134_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &imx134_s_ctrl);
}
static struct i2c_driver imx134_i2c_driver = {
	.id_table = imx134_i2c_id,
	.probe  = msm_imx134_i2c_probe,
	.driver = {
		.name = IMX134_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx134_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx134_dt_match[] = {
	{.compatible = "qcom,imx134", .data = &imx134_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, imx134_dt_match);

static struct platform_driver imx134_platform_driver = {
	.driver = {
		.name = "qcom,imx134",
		.owner = THIS_MODULE,
		.of_match_table = imx134_dt_match,
	},
};

extern uint16_t back_cam_fw_version;

static ssize_t back_camera_type_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	char type[] = "SONY_IMX134_FIMC_IS\n";

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
	char cam_fw[] = "A08QSGF01AA A08QSGF01AA\n";/* PowerLogics_module, 8mega_pixel, Qualcomm_isp, Sony_sensor*/

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

static int32_t imx134_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	struct device 	*cam_dev_back;
	struct device 	*cam_dev_front;
	match = of_match_device(imx134_dt_match, &pdev->dev);
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

static int __init imx134_init_module(void)
{
	int32_t rc = 0;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&imx134_platform_driver,
		imx134_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&imx134_i2c_driver);
}

static void __exit imx134_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (imx134_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx134_s_ctrl);
		platform_driver_unregister(&imx134_platform_driver);
	} else
		i2c_del_driver(&imx134_i2c_driver);
	return;
}

static struct msm_sensor_ctrl_t imx134_s_ctrl = {
	.sensor_i2c_client = &imx134_sensor_i2c_client,
	.msm_sensor_mutex = &imx134_mut,
	.sensor_v4l2_subdev_info = imx134_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx134_subdev_info),
};

module_init(imx134_init_module);
module_exit(imx134_exit_module);
MODULE_DESCRIPTION("imx134");
MODULE_LICENSE("GPL v2");
