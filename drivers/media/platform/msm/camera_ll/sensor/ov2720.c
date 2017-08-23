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
#define OV2720_SENSOR_NAME "ov2720"
#undef CDBG
#ifdef OV2720_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#endif
DEFINE_MSM_MUTEX(ov2720_mut);

static struct msm_sensor_ctrl_t ov2720_s_ctrl;


static struct v4l2_subdev_info ov2720_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id ov2720_i2c_id[] = {
	{OV2720_SENSOR_NAME, (kernel_ulong_t)&ov2720_s_ctrl},
	{ }
};

static int32_t msm_ov2720_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &ov2720_s_ctrl);
}
static struct i2c_driver ov2720_i2c_driver = {
	.id_table = ov2720_i2c_id,
	.probe  = msm_ov2720_i2c_probe,
	.driver = {
		.name = OV2720_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov2720_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id ov2720_dt_match[] = {
	{.compatible = "qcom,ov2720", .data = &ov2720_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, ov2720_dt_match);

static struct platform_driver ov2720_platform_driver = {
	.driver = {
		.name = "qcom,ov2720",
		.owner = THIS_MODULE,
		.of_match_table = ov2720_dt_match,
	},
};

static int32_t ov2720_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(ov2720_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init ov2720_init_module(void)
{
	int32_t rc = 0;
	CDBG("%s:%d init\n", __func__, __LINE__);
	rc = platform_driver_probe(&ov2720_platform_driver,
		ov2720_platform_probe);
	if (!rc) {
		pr_info("%s: probe success\n", __func__);
		return rc;
	}
	return i2c_add_driver(&ov2720_i2c_driver);
}

static void __exit ov2720_exit_module(void)
{
	CDBG("%s:%d\n", __func__, __LINE__);
	if (ov2720_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&ov2720_s_ctrl);
		platform_driver_unregister(&ov2720_platform_driver);
	} else
		i2c_del_driver(&ov2720_i2c_driver);
	return;
}

static struct msm_sensor_ctrl_t ov2720_s_ctrl = {
	.sensor_i2c_client = &ov2720_sensor_i2c_client,
	.msm_sensor_mutex = &ov2720_mut,
	.sensor_v4l2_subdev_info = ov2720_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov2720_subdev_info),
};

module_init(ov2720_init_module);
module_exit(ov2720_exit_module);
MODULE_DESCRIPTION("ov2720");
MODULE_LICENSE("GPL v2");
