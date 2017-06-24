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
#include <linux/of.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/ioctl.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/videodev2.h>
#include <linux/msm_ion.h>
#include <linux/iommu.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <media/v4l2-fh.h>
#include "msm.h"
#include "msm_vb2.h"
#include "msm_sd.h"

static struct v4l2_device *msm_v4l2_dev;
/* static struct msm_cam_dummy_queue cam_dummy_queue; */

static int msm_open_cam_dummy(struct file *fp)
{
	int rc;

	pr_err("%s: E\n", __func__);
	rc = msm_cam_get_module_init_status();
	pr_err("%s: X %d\n", __func__, rc);
	return rc;
}

static long msm_ioctl_cam_dummy(struct file *fp, unsigned int cmd,
	unsigned long arg)
{
	return 0;
}

static int msm_close_cam_dummy(struct file *f)
{
	return 0;
}

static struct v4l2_file_operations msm_fops_config = {
	.owner  = THIS_MODULE,
	.open  = msm_open_cam_dummy,
	.release = msm_close_cam_dummy,
	.unlocked_ioctl = msm_ioctl_cam_dummy,
};

static const struct of_device_id cam_dummy_dt_match[] = {
	{.compatible = "qcom,cam_dummy",},
	{}
};

MODULE_DEVICE_TABLE(of, cam_dummy_dt_match);

static struct platform_driver cam_dummy_platform_driver = {
	.driver = {
		.name = "qcom,cam_dummy",
		.owner = THIS_MODULE,
		.of_match_table = cam_dummy_dt_match,
	},
};

static int32_t cam_dummy_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	struct msm_video_device *pvdev;

	/* init_waitqueue_head(&cam_dummy_queue.state_wait);*/
	pr_err("%s:%d\n", __func__, __LINE__);
	match = of_match_device(cam_dummy_dt_match, &pdev->dev);

	msm_v4l2_dev = kzalloc(sizeof(*msm_v4l2_dev),
		GFP_KERNEL);
	if (WARN_ON(!msm_v4l2_dev)) {
		rc = -ENOMEM;
		goto probe_end;
	}

	pvdev = kzalloc(sizeof(struct msm_video_device),
		GFP_KERNEL);
	if (WARN_ON(!pvdev)) {
		rc = -ENOMEM;
		goto pvdev_fail;
	}

	pvdev->vdev = video_device_alloc();
	if (WARN_ON(!pvdev->vdev)) {
		rc = -ENOMEM;
		goto video_fail;
	}

#if defined(CONFIG_MEDIA_CONTROLLER)
	msm_v4l2_dev->mdev = kzalloc(sizeof(struct media_device),
		GFP_KERNEL);
	if (!msm_v4l2_dev->mdev) {
		rc = -ENOMEM;
		goto mdev_fail;
	}
	strlcpy(msm_v4l2_dev->mdev->model, MSM_CAMERA_DUMMY_NAME,
			sizeof(msm_v4l2_dev->mdev->model));
	msm_v4l2_dev->mdev->dev = &(pdev->dev);

	rc = media_device_register(msm_v4l2_dev->mdev);
	if (WARN_ON(rc < 0))
		goto media_fail;

	if (WARN_ON((rc == media_entity_init(&pvdev->vdev->entity,
			0, NULL, 0)) < 0))
		goto entity_fail;

	pvdev->vdev->entity.type = MEDIA_ENT_T_DEVNODE_V4L;
	pvdev->vdev->entity.group_id = QCAMERA_VNODE_GROUP_ID;
#endif

	pvdev->vdev->v4l2_dev = msm_v4l2_dev;

	rc = v4l2_device_register(&(pdev->dev), pvdev->vdev->v4l2_dev);
	if (WARN_ON(rc < 0))
		goto register_fail;

	strlcpy(pvdev->vdev->name, "msm-camdummy", sizeof(pvdev->vdev->name));
	pvdev->vdev->release  = video_device_release;
	pvdev->vdev->fops     = &msm_fops_config;
	pvdev->vdev->minor     = -1;
	pvdev->vdev->vfl_type  = VFL_TYPE_GRABBER;
	rc = video_register_device(pvdev->vdev,
		VFL_TYPE_GRABBER, -1);
	if (WARN_ON(rc < 0))
		goto v4l2_fail;

#if defined(CONFIG_MEDIA_CONTROLLER)
	/* FIXME: How to get rid of this messy? */
	pvdev->vdev->entity.name = video_device_node_name(pvdev->vdev);
#endif

	atomic_set(&pvdev->opened, 0);
	video_set_drvdata(pvdev->vdev, pvdev);

	goto probe_end;

v4l2_fail:
	v4l2_device_unregister(pvdev->vdev->v4l2_dev);
register_fail:
#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&pvdev->vdev->entity);
entity_fail:
	media_device_unregister(msm_v4l2_dev->mdev);
media_fail:
	kzfree(msm_v4l2_dev->mdev);
mdev_fail:
#endif
	video_device_release(pvdev->vdev);
video_fail:
	kzfree(pvdev);
pvdev_fail:
	kzfree(msm_v4l2_dev);
probe_end:
	return rc;
}

static int __init cam_dummy_init_module(void)
{
	int32_t rc = 0;
	pr_err("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&cam_dummy_platform_driver,
		cam_dummy_platform_probe);

	if (rc < 0)
	  pr_err("%s: failed", __func__);
	else
	  pr_warn("%s: succeed", __func__);

	return rc;
}

static void __exit cam_dummy_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	platform_driver_unregister(&cam_dummy_platform_driver);
	return;
}

module_init(cam_dummy_init_module);
module_exit(cam_dummy_exit_module);
MODULE_DESCRIPTION("cam_dummy");
MODULE_LICENSE("GPL v2");

