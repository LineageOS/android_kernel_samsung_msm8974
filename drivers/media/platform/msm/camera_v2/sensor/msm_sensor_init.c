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
 */

#define pr_fmt(fmt) "MSM-SENSOR-INIT %s:%d " fmt "\n", __func__, __LINE__

/* Header files */
#include <mach/gpiomux.h>
#include "msm_sensor_init.h"
#include "msm_sensor_driver.h"
#include "msm_sensor.h"
#include "msm_sd.h"

//#define CONFIG_MSMB_CAMERA_DEBUG
/* Logging macro */
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args ...) pr_err(fmt, ## args)
#else
#define CDBG(fmt, args ...) do { } while (0)
#endif

struct class *camera_class;

/* Static function declaration */
static long msm_sensor_init_subdev_ioctl(struct v4l2_subdev *sd,
					 unsigned int cmd, void *arg);

/* Static structure declaration */
static struct v4l2_subdev_core_ops msm_sensor_init_subdev_core_ops = {
	.ioctl	= msm_sensor_init_subdev_ioctl,
};

static struct v4l2_subdev_ops msm_sensor_init_subdev_ops = {
	.core	= &msm_sensor_init_subdev_core_ops,
};

static const struct v4l2_subdev_internal_ops msm_sensor_init_internal_ops;

/* Static function definition */
static long msm_sensor_driver_cmd(struct msm_sensor_init_t *s_init, void *arg)
{
	int32_t rc = 0;
	struct sensor_init_cfg_data *cfg = (struct sensor_init_cfg_data *)arg;

	/* Validate input parameters */
	if (!s_init || !cfg) {
		pr_err("failed: s_init %pK cfg %pK", s_init, cfg);
		return -EINVAL;
	}

	switch (cfg->cfgtype) {
	case CFG_SINIT_PROBE:
		pr_warn("%s : CFG_SINIT_PROBE", __func__);
		rc = msm_sensor_driver_probe(cfg->cfg.setting);
		if (rc < 0)
			pr_err("failed: msm_sensor_driver_probe rc %d", rc);
		break;
	default:
		pr_err("%s : default", __func__);
		break;
	}

	return rc;
}

static long msm_sensor_init_subdev_ioctl(struct v4l2_subdev *sd,
					 unsigned int cmd, void *arg)
{
	int32_t rc = 0;
	struct msm_sensor_init_t *s_init = v4l2_get_subdevdata(sd);

	CDBG("Enter");

	/* Validate input parameters */
	if (!s_init) {
		pr_err("failed: s_init %pK", s_init);
		return -EINVAL;
	}

	switch (cmd) {
	case VIDIOC_MSM_SENSOR_INIT_CFG:
		rc = msm_sensor_driver_cmd(s_init, arg);
		break;

	default:
		pr_err("default");
		break;
	}
	return 0;

}

extern uint16_t back_cam_fw_version;
char cam_core_ver[3] = "L0"; //To check 2P2 version (AVDD 2.8V or AVDD 2.95V) : [0]sensor_maker, [1]core_ver

static ssize_t back_camera_type_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	char type_lsi[] = "LSI_S5K2P2XX\n";
	char type_sony[] = "SONY_IMX240\n";

	if (cam_core_ver[0] == 'L') {
		return snprintf(buf, sizeof(type_lsi), "%s", type_lsi);
	} else {
		return snprintf(buf, sizeof(type_sony), "%s", type_sony);
	}

}

static ssize_t front_camera_type_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	char cam_type[] = "S5K8B1YX\n";

	return snprintf(buf, sizeof(cam_type), "%s", cam_type);
}


char cam_fw_ver[40] = "NULL NULL\n";//multi module
static ssize_t back_camera_firmware_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] cam_fw_ver : %s\n", cam_fw_ver);
	return snprintf(buf, sizeof(cam_fw_ver), "%s", cam_fw_ver);
}

static ssize_t back_camera_firmware_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(cam_fw_ver, sizeof(cam_fw_ver), "%s", buf);

	return size;
}

char cam_fw_full_ver[40] = "NULL NULL NULL\n";//multi module
static ssize_t back_camera_firmware_full_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] cam_fw_ver : %s\n", cam_fw_full_ver);
	return snprintf(buf, sizeof(cam_fw_full_ver), "%s", cam_fw_full_ver);
}

static ssize_t back_camera_firmware_full_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(cam_fw_full_ver, sizeof(cam_fw_full_ver), "%s", buf);

	return size;
}

char cam_load_fw[25] = "NULL\n";
static ssize_t back_camera_firmware_load_show(struct device *dev,
					      struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] cam_load_fw : %s\n", cam_load_fw);
	return snprintf(buf, sizeof(cam_load_fw), "%s", cam_load_fw);
}

static ssize_t back_camera_firmware_load_store(struct device *dev,
					       struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(cam_load_fw, sizeof(cam_load_fw), "%s\n", buf);
	return size;
}

char cam_latest_check[3] = "NG\n";
static ssize_t back_camera_latest_module_check_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] cam_latest_check : %s\n", cam_latest_check);
	return snprintf(buf, sizeof(cam_latest_check), "%s", cam_latest_check);
}

static ssize_t back_camera_latest_module_check_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(cam_latest_check, sizeof(cam_latest_check), "%s", buf);
	return size;
}


static ssize_t back_camera_core_version_show(struct device *dev,
					     struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] cam_core_ver : %s\n", cam_core_ver);
	return snprintf(buf, sizeof(cam_core_ver), "%s", cam_core_ver);
}

static ssize_t back_camera_core_version_store(struct device *dev,
					      struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(cam_core_ver, sizeof(cam_core_ver), "%s\n", buf);
	return size;
}

#ifdef CONFIG_COMPANION
char companion_fw_ver[40] = "NULL NULL NULL\n";
static ssize_t back_companion_firmware_show(struct device *dev,
					    struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] companion_fw_ver : %s\n", companion_fw_ver);
	return snprintf(buf, sizeof(companion_fw_ver), "%s", companion_fw_ver);
}

static ssize_t back_companion_firmware_store(struct device *dev,
					     struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(companion_fw_ver, sizeof(companion_fw_ver), "%s", buf);

	return size;
}
#endif

char fw_crc[10] = "NN\n"; //camera and companion
static ssize_t back_fw_crc_check_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] fw_crc : %s\n", fw_crc);
	return snprintf(buf, sizeof(fw_crc), "%s", fw_crc);
}

static ssize_t back_fw_crc_check_store(struct device *dev,
				       struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(fw_crc, sizeof(fw_crc), "%s", buf);

	return size;
}

char cal_crc[37] = "NULL NULL NULL\n";
static ssize_t back_cal_data_check_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] cal_crc : %s\n", cal_crc);
	return snprintf(buf, sizeof(cal_crc), "%s", cal_crc);
}

static ssize_t back_cal_data_check_store(struct device *dev,
					 struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(cal_crc, sizeof(cal_crc), "%s", buf);

	return size;
}

char isp_core[10];
static ssize_t back_isp_core_check_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	CDBG("[FW_DBG] isp_core : %s\n", isp_core);
	return snprintf(buf, sizeof(isp_core), "%s\n", isp_core);
}

static ssize_t back_isp_core_check_store(struct device *dev,
					 struct device_attribute *attr, const char *buf, size_t size)
{
	CDBG("[FW_DBG] buf : %s\n", buf);
	snprintf(isp_core, sizeof(isp_core), "%s", buf);

	return size;
}

static ssize_t front_camera_firmware_show(struct device *dev,
					  struct device_attribute *attr, char *buf)
{
	char cam_fw[] = "S5K8B1YX N\n";

	return snprintf(buf, sizeof(cam_fw), "%s", cam_fw);
}

static DEVICE_ATTR(rear_camtype, S_IRUGO, back_camera_type_show, NULL);
static DEVICE_ATTR(rear_camfw, S_IRUGO|S_IWUSR|S_IWGRP,
    back_camera_firmware_show, back_camera_firmware_store);
static DEVICE_ATTR(rear_camfw_full, S_IRUGO|S_IWUSR|S_IWGRP,
    back_camera_firmware_full_show, back_camera_firmware_full_store);
static DEVICE_ATTR(rear_camfw_load, S_IRUGO|S_IWUSR|S_IWGRP,
    back_camera_firmware_load_show, back_camera_firmware_load_store);
static DEVICE_ATTR(rear_latest_module_check, S_IRUGO|S_IWUSR|S_IWGRP,
    back_camera_latest_module_check_show, back_camera_latest_module_check_store);
static DEVICE_ATTR(rear_corever, S_IRUGO|S_IWUSR|S_IWGRP,
    back_camera_core_version_show, back_camera_core_version_store);
#ifdef CONFIG_COMPANION
static DEVICE_ATTR(rear_companionfw_full, S_IRUGO | S_IWUSR | S_IWGRP,
		   back_companion_firmware_show, back_companion_firmware_store);
#endif
static DEVICE_ATTR(rear_fwcheck, S_IRUGO | S_IWUSR | S_IWGRP,
		   back_fw_crc_check_show, back_fw_crc_check_store);
static DEVICE_ATTR(rear_calcheck, S_IRUGO | S_IWUSR | S_IWGRP,
		   back_cal_data_check_show, back_cal_data_check_store);
static DEVICE_ATTR(isp_core, S_IRUGO | S_IWUSR | S_IWGRP,
		   back_isp_core_check_show, back_isp_core_check_store);
static DEVICE_ATTR(front_camtype, S_IRUGO, front_camera_type_show, NULL);
static DEVICE_ATTR(front_camfw, S_IRUGO, front_camera_firmware_show, NULL);

static int __init msm_sensor_init_module(void)
{
	struct msm_sensor_init_t *s_init = NULL;
	struct device            *cam_dev_back;
	struct device            *cam_dev_front;

	int rc = 0;
	camera_class = class_create(THIS_MODULE, "camera");
	if (IS_ERR(camera_class))
		pr_err("failed to create device cam_dev_rear!\n");

	/* Allocate memory for msm_sensor_init control structure */
	s_init = kzalloc(sizeof(struct msm_sensor_init_t), GFP_KERNEL);
	if (!s_init) {
		class_destroy(camera_class);
		pr_err("failed: no memory s_init %pK", NULL);
		return -ENOMEM;
	}

	/* Initialize mutex */
	mutex_init(&s_init->imutex);

	/* Create /dev/v4l-subdevX for msm_sensor_init */
	v4l2_subdev_init(&s_init->msm_sd.sd, &msm_sensor_init_subdev_ops);
	snprintf(s_init->msm_sd.sd.name, sizeof(s_init->msm_sd.sd.name), "%s",
		 "msm_sensor_init");
	v4l2_set_subdevdata(&s_init->msm_sd.sd, s_init);
	s_init->msm_sd.sd.internal_ops = &msm_sensor_init_internal_ops;
	s_init->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	rc = media_entity_init(&s_init->msm_sd.sd.entity, 0, NULL, 0);
	if (rc < 0) {
		printk("Failed to media entity init!\n");
		goto entity_fail;
	}
	s_init->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_init->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_SENSOR_INIT;
	s_init->msm_sd.sd.entity.name = s_init->msm_sd.sd.name;
	s_init->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x7;
	rc = msm_sd_register(&s_init->msm_sd);
	if (rc < 0) {
		printk("Failed to msms sd register!\n");
		goto msm_sd_register_fail;
	}

	cam_dev_back = device_create(camera_class, NULL,
				     1, NULL, "rear");
	if (IS_ERR(cam_dev_back)) {
		printk("Failed to create cam_dev_back device!\n");
		rc = -ENODEV;
		goto device_create_fail;
	}

	if (device_create_file(cam_dev_back, &dev_attr_rear_camtype) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_camtype.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_camfw) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_camfw.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_camfw_full) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_camfw_full.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_camfw_load) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_camfw_load.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_latest_module_check) < 0) {
		printk("Failed to create device file!(%s)!\n",
			dev_attr_rear_latest_module_check.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_corever) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_corever.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
#ifdef CONFIG_COMPANION
	if (device_create_file(cam_dev_back, &dev_attr_rear_companionfw_full) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_companionfw_full.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
#endif
	if (device_create_file(cam_dev_back, &dev_attr_rear_fwcheck) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_fwcheck.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_rear_calcheck) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_rear_calcheck.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_back, &dev_attr_isp_core) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_isp_core.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}

	cam_dev_front = device_create(camera_class, NULL,
				      2, NULL, "front");
	if (IS_ERR(cam_dev_front)) {
		printk("Failed to create cam_dev_front device!");
		rc = -ENODEV;
		goto device_create_fail;
	}

	if (device_create_file(cam_dev_front, &dev_attr_front_camtype) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_front_camtype.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}
	if (device_create_file(cam_dev_front, &dev_attr_front_camfw) < 0) {
		printk("Failed to create device file!(%s)!\n",
		       dev_attr_front_camfw.attr.name);
		rc = -ENODEV;
		goto device_create_fail;
	}

	return 0;

device_create_fail:
	msm_sd_unregister(&s_init->msm_sd);
msm_sd_register_fail:
	media_entity_cleanup(&s_init->msm_sd.sd.entity);
entity_fail:
	mutex_destroy(&s_init->imutex);
	kfree(s_init);
	class_destroy(camera_class);
	return rc;
}

static void __exit msm_sensor_exit_module(void)
{
	return;
}

module_init(msm_sensor_init_module);
module_exit(msm_sensor_exit_module);
MODULE_DESCRIPTION("msm_sensor_init");
MODULE_LICENSE("GPL v2");
