/* Copyright (c) 2013-2015,2017 The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt) "MSM-SENSOR-DRIVER %s:%d " fmt "\n", __func__, __LINE__

/* Header file declaration */
#include "msm_sensor.h"
#include "msm_sd.h"
#include "camera.h"
#include "msm_cci.h"
#include "msm_camera_dt_util.h"

//#define MSM_SENSOR_DRIVER_DEBUG
/* Logging macro */
#undef CDBG
#ifdef MSM_SENSOR_DRIVER_DEBUG
#define CDBG(fmt, args ...) pr_err(fmt, ## args)
#else
#define CDBG(fmt, args ...) pr_debug(fmt, ## args)
#endif

/* Static declaration */
static struct msm_sensor_ctrl_t *g_sctrl[MAX_CAMERAS];

static const struct of_device_id msm_sensor_driver_dt_match[] = {
	{ .compatible = "qcom,camera" },
	{}
};

MODULE_DEVICE_TABLE(of, msm_sensor_driver_dt_match);

static struct platform_driver msm_sensor_platform_driver = {
	.driver			= {
		.name		= "qcom,camera",
		.owner		= THIS_MODULE,
		.of_match_table = msm_sensor_driver_dt_match,
	},
};

static struct v4l2_subdev_info msm_sensor_driver_subdev_info[] = {
	{
		.code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt = 1,
		.order = 0,
	},
};

/* static function definition */
int32_t msm_sensor_driver_probe(void *setting)
{
	int32_t rc = 0;
	int32_t is_power_off = 0;
	uint16_t i = 0, size = 0, off_size = 0;
	uint32_t session_id = 0;
	struct msm_sensor_ctrl_t            *s_ctrl = NULL;
	struct msm_camera_cci_client        *cci_client = NULL;
	struct msm_camera_sensor_slave_info *slave_info = NULL;
	struct msm_sensor_power_setting     *power_setting = NULL;
	struct msm_sensor_power_setting     *power_off_setting = NULL;
	struct msm_camera_slave_info        *camera_info = NULL;
	struct msm_camera_power_ctrl_t      *power_info = NULL;

	/* Validate input parameters */
	if (!setting) {
		pr_err("failed: slave_info %pK", setting);
		return -EINVAL;
	}

	/* Allocate memory for slave info */
	slave_info = kzalloc(sizeof(*slave_info), GFP_KERNEL);
	if (!slave_info) 
		return -ENOMEM;
	

	if (copy_from_user(slave_info, (void*)setting, sizeof(*slave_info))) {
		pr_err("failed: copy_from_user");
		rc = -EFAULT;
		goto FREE_SLAVE_INFO;
	}

	/* Print slave info */
	CDBG("camera id %d", slave_info->camera_id);
	CDBG("slave_addr %x", slave_info->slave_addr);
	CDBG("addr_type %d", slave_info->addr_type);
	CDBG("sensor_id_reg_addr %x",
	     slave_info->sensor_id_info.sensor_id_reg_addr);
	CDBG("sensor_id %x", slave_info->sensor_id_info.sensor_id);
	CDBG("size %x", slave_info->power_setting_array.size);

	/* Validate camera id */
	if (slave_info->camera_id >= MAX_CAMERAS) {
		pr_err("failed: invalid camera id %d max %d",
		       slave_info->camera_id, MAX_CAMERAS);
		rc = -EINVAL;
		goto FREE_SLAVE_INFO;
	}

	/* Extract s_ctrl from camera id */
	s_ctrl = g_sctrl[slave_info->camera_id];
	if (!s_ctrl) {
		pr_err("failed: s_ctrl %pK for camera_id %d", s_ctrl,
		       slave_info->camera_id);
		rc = -EINVAL;
		goto FREE_SLAVE_INFO;
	}

	CDBG("s_ctrl[%d] %pK", slave_info->camera_id, s_ctrl);

	if (s_ctrl->is_probe_succeed == 1) {
		/*
		 * Different sensor on this camera slot has been connected
		 * and probe already succeeded for that sensor. Ignore this
		 * probe
		 */
		pr_err("slot %d has some other sensor", slave_info->camera_id);
		kfree(slave_info);
		return 0;
	}

	size = slave_info->power_setting_array.size;
	/* Allocate memory for power setting */
	power_setting = kzalloc(sizeof(*power_setting) * size, GFP_KERNEL);
	if (!power_setting) {
		pr_err("failed: no memory power_setting %p", power_setting);
		rc = -ENOMEM;
		goto FREE_SLAVE_INFO;
	}

	if (copy_from_user(power_setting,
			   (void*)slave_info->power_setting_array.power_setting,
			   sizeof(*power_setting) * size)) {
		pr_err("failed: copy_from_user");
		rc = -EFAULT;
		goto FREE_POWER_SETTING;
	}

	/* Print power setting */
	for (i = 0; i < size; i++) {
		CDBG("seq_type %d seq_val %d config_val %ld delay %d",
		     power_setting[i].seq_type, power_setting[i].seq_val,
		     power_setting[i].config_val, power_setting[i].delay);
	}

	off_size = slave_info->power_setting_array.off_size;
	if (off_size > 0) {
		/* Allocate memory for power setting */
		power_off_setting = kzalloc(sizeof(*power_off_setting) * off_size, GFP_KERNEL);
		if (!power_off_setting) {
			pr_err("failed: no memory power_setting %p", power_off_setting);
			rc = -ENOMEM;
			goto FREE_POWER_SETTING;
		}

		if (copy_from_user(power_off_setting,
				   (void*)slave_info->power_setting_array.power_off_setting,
				   sizeof(*power_off_setting) * off_size)) {
			pr_err("failed: copy_from_user");
			rc = -EFAULT;
			goto FREE_POWER_OFF_SETTING;
		}

		/* Print power setting */
		for (i = 0; i < off_size; i++) {
			CDBG("seq_type %d seq_val %d config_val %ld delay %d",
			     power_off_setting[i].seq_type, power_off_setting[i].seq_val,
			     power_off_setting[i].config_val, power_off_setting[i].delay);
		}
		is_power_off = 1;
	}

	camera_info = kzalloc(sizeof(struct msm_camera_slave_info), GFP_KERNEL);
	if (!camera_info) {
		if (is_power_off)
			goto FREE_POWER_OFF_SETTING;
		else
			goto FREE_POWER_SETTING;
	}

	/* Fill power up setting and power up setting size */
	power_info = &s_ctrl->sensordata->power_info;
	power_info->power_setting = power_setting;
	power_info->power_setting_size = size;
	power_info->power_off_setting = power_off_setting;
	power_info->power_off_setting_size = off_size;

	s_ctrl->sensordata->slave_info = camera_info;
	s_ctrl->sensor_device_type = MSM_CAMERA_PLATFORM_DEVICE;

	/* Fill sensor slave info */
	camera_info->sensor_slave_addr = slave_info->slave_addr;
	camera_info->sensor_id_reg_addr =
		slave_info->sensor_id_info.sensor_id_reg_addr;
	camera_info->sensor_id = slave_info->sensor_id_info.sensor_id;

	/* Fill CCI master, slave address and CCI default params */
	if (!s_ctrl->sensor_i2c_client) {
		pr_err("failed: sensor_i2c_client %pK",
		       s_ctrl->sensor_i2c_client);
		rc = -EINVAL;
		if (is_power_off)
			goto FREE_POWER_OFF_SETTING;
		else
			goto FREE_POWER_SETTING;
	}
	/* Fill sensor address type */
	s_ctrl->sensor_i2c_client->addr_type = slave_info->addr_type;

	cci_client = s_ctrl->sensor_i2c_client->cci_client;
	if (!cci_client) {
		pr_err("failed: cci_client %pK", cci_client);
		if (is_power_off)
			goto FREE_POWER_OFF_SETTING;
		else
			goto FREE_POWER_SETTING;
	}
	cci_client->cci_i2c_master = s_ctrl->cci_i2c_master;
	cci_client->sid = slave_info->slave_addr >> 1;
	cci_client->retries = 3;
	cci_client->id_map = 0;

	/* Parse and fill vreg params */
	rc = msm_camera_fill_vreg_params(
		power_info->cam_vreg,
		power_info->num_vreg,
		power_info->power_setting,
		power_info->power_setting_size);
	if (rc < 0) {
		pr_err("failed: msm_camera_get_dt_power_setting_data rc %d",
		       rc);
		if (is_power_off)
			goto FREE_POWER_OFF_SETTING;
		else
			goto FREE_POWER_SETTING;
	}

	if (power_info->power_off_setting && (power_info->power_off_setting_size > 0)) {
		/* Parse and fill vreg params */
		rc = msm_camera_fill_vreg_params(
			power_info->cam_vreg,
			power_info->num_vreg,
			power_info->power_off_setting,
			power_info->power_off_setting_size);
		if (rc < 0) {
			pr_err("failed: msm_camera_get_dt_power_setting_data rc %d",
			       rc);
			if (is_power_off)
				goto FREE_POWER_OFF_SETTING;
			else
				goto FREE_POWER_SETTING;
		}
	}
	/* remove this code for DFMS test */
#if 0
	/* Power up and probe sensor */
	rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl,
					       &s_ctrl->sensordata->power_info,
					       s_ctrl->sensor_i2c_client,
					       s_ctrl->sensordata->slave_info,
					       slave_info->sensor_name);
	if (rc < 0) {
		pr_err("%s power up failed", slave_info->sensor_name);
		if (is_power_off)
			goto FREE_POWER_OFF_SETTING;
		else
			goto FREE_POWER_SETTING;
	}
#endif

	/* Update sensor name in sensor control structure */
	s_ctrl->sensordata->sensor_name = slave_info->sensor_name;

	/*
	 * Create /dev/videoX node, comment for now until dummy /dev/videoX
	 * node is created and used by HAL
	 */
	rc = camera_init_v4l2(&s_ctrl->pdev->dev, &session_id);
	if (rc < 0) {
		pr_err("failed: camera_init_v4l2 rc %d", rc);
		if (is_power_off)
			goto FREE_POWER_OFF_SETTING;
		else
			goto FREE_POWER_SETTING;
	}
	s_ctrl->sensordata->sensor_info->session_id = session_id;

	/* Create /dev/v4l-subdevX device */
	v4l2_subdev_init(&s_ctrl->msm_sd.sd, s_ctrl->sensor_v4l2_subdev_ops);
	snprintf(s_ctrl->msm_sd.sd.name, sizeof(s_ctrl->msm_sd.sd.name), "%s",
		 s_ctrl->sensordata->sensor_name);
	v4l2_set_subdevdata(&s_ctrl->msm_sd.sd, s_ctrl->pdev);
	s_ctrl->msm_sd.sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	media_entity_init(&s_ctrl->msm_sd.sd.entity, 0, NULL, 0);
	s_ctrl->msm_sd.sd.entity.type = MEDIA_ENT_T_V4L2_SUBDEV;
	s_ctrl->msm_sd.sd.entity.group_id = MSM_CAMERA_SUBDEV_SENSOR;
	s_ctrl->msm_sd.sd.entity.name = s_ctrl->msm_sd.sd.name;
	s_ctrl->msm_sd.close_seq = MSM_SD_CLOSE_2ND_CATEGORY | 0x3;
	rc = msm_sd_register(&s_ctrl->msm_sd);
	if (rc < 0) {
		pr_err("failed: msm_sd_register rc %d", rc);
		return rc;
	}

	memcpy(slave_info->subdev_name, s_ctrl->msm_sd.sd.entity.name,
	       sizeof(slave_info->subdev_name));
	slave_info->is_probe_succeed = 1;

	slave_info->sensor_info.session_id =
		s_ctrl->sensordata->sensor_info->session_id;
	for (i = 0; i < SUB_MODULE_MAX; i++) {
		slave_info->sensor_info.subdev_id[i] =
			s_ctrl->sensordata->sensor_info->subdev_id[i];
		slave_info->sensor_info.subdev_intf[i] =
			s_ctrl->sensordata->sensor_info->subdev_intf[i];
	}
	slave_info->sensor_info.is_mount_angle_valid =
		s_ctrl->sensordata->sensor_info->is_mount_angle_valid;
	slave_info->sensor_info.sensor_mount_angle =
		s_ctrl->sensordata->sensor_info->sensor_mount_angle;
	CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
	     slave_info->sensor_info.sensor_name);
	CDBG("%s:%d session id %d\n", __func__, __LINE__,
	     slave_info->sensor_info.session_id);
	for (i = 0; i < SUB_MODULE_MAX; i++) {
		/*
		   pr_err("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
		   slave_info->sensor_info.subdev_id[i]);
		   pr_err("%s:%d additional subdev_intf[%d] %d\n", __func__, __LINE__, i,
		   slave_info->sensor_info.subdev_intf[i]);
		 */
	}
	CDBG("%s:%d mount angle valid %d value %d\n", __func__,
	     __LINE__, slave_info->sensor_info.is_mount_angle_valid,
	     slave_info->sensor_info.sensor_mount_angle);

	if (copy_to_user((void __user*)setting,
			 (void*)slave_info, sizeof(*slave_info))) {
		pr_err("%s:%d copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}

	pr_warn("rc %d session_id %d", rc, session_id);
	pr_warn("%s probe succeeded", slave_info->sensor_name);

	/* remove this code for DFMS test */
#if 0
	/* Power down */
	s_ctrl->func_tbl->sensor_power_down(
		s_ctrl,
		&s_ctrl->sensordata->power_info,
		s_ctrl->sensor_device_type,
		s_ctrl->sensor_i2c_client);
#endif

	/*COMP_EN init-set low*/
	gpio_set_value_cansleep(
		power_info->gpio_conf->gpio_num_info->gpio_num
		[SENSOR_GPIO_COMP], GPIOF_OUT_INIT_LOW);

	/*
	   Set probe succeeded flag to 1 so that no other camera shall
	 * probed on this slot
	 */
	s_ctrl->is_probe_succeed = 1;
	return rc;

 FREE_POWER_OFF_SETTING:
	kfree(power_off_setting);
 FREE_POWER_SETTING:
	kfree(power_setting);
 FREE_SLAVE_INFO:
	kfree(slave_info);
	return rc;
}

static int32_t msm_sensor_driver_get_gpio_data(
	struct msm_camera_sensor_board_info *sensordata,
	struct device_node *of_node)
{
	int32_t rc = 0, i = 0;
	struct msm_camera_gpio_conf *gconf = NULL;
	uint16_t                    *gpio_array = NULL;
	uint16_t gpio_array_size = 0;

	/* Validate input paramters */
	if (!sensordata || !of_node) {
		pr_err("failed: invalid params sensordata %p of_node %p",
		       sensordata, of_node);
		return -EINVAL;
	}

	sensordata->power_info.gpio_conf = kzalloc(
		sizeof(struct msm_camera_gpio_conf), GFP_KERNEL);
	if (!sensordata->power_info.gpio_conf) {
		pr_err("failed");
		return -ENOMEM;
	}
	gconf = sensordata->power_info.gpio_conf;

	gpio_array_size = of_gpio_count(of_node);
	CDBG("gpio count %d", gpio_array_size);
	if (!gpio_array_size)
		return 0;

	gpio_array = kzalloc(sizeof(uint16_t) * gpio_array_size, GFP_KERNEL);
	if (!gpio_array) {
		pr_err("failed");
		goto FREE_GPIO_CONF;
	}
	for (i = 0; i < gpio_array_size; i++) {
		gpio_array[i] = of_get_gpio(of_node, i);
		CDBG("gpio_array[%d] = %d", i, gpio_array[i]);
	}

	rc = msm_camera_get_dt_gpio_req_tbl(of_node, gconf, gpio_array,
					    gpio_array_size);
	if (rc < 0) {
		pr_err("failed");
		goto FREE_GPIO_CONF;
	}

	rc = msm_camera_init_gpio_pin_tbl(of_node, gconf, gpio_array,
					  gpio_array_size);
	if (rc < 0) {
		pr_err("failed");
		goto FREE_GPIO_REQ_TBL;
	}

	kfree(gpio_array);
	return rc;

 FREE_GPIO_REQ_TBL:
	kfree(sensordata->power_info.gpio_conf->cam_gpio_req_tbl);
 FREE_GPIO_CONF:
	kfree(sensordata->power_info.gpio_conf);
	kfree(gpio_array);
	return rc;
}

static int32_t msm_sensor_driver_get_dt_data(struct msm_sensor_ctrl_t *s_ctrl,
					     struct platform_device *pdev)
{
	int32_t rc = 0;
	struct msm_camera_sensor_board_info *sensordata = NULL;
	struct device_node                  *of_node = pdev->dev.of_node;

	s_ctrl->sensordata = kzalloc(sizeof(*sensordata), GFP_KERNEL);
	if (!s_ctrl->sensordata) {
		pr_err("failed: no memory");
		return -ENOMEM;
	}

	sensordata = s_ctrl->sensordata;


	/*
	 * Read cell index - this cell index will be the camera slot where
	 * this camera will be mounted
	 */
	rc = of_property_read_u32(of_node, "cell-index", &pdev->id);
	if (rc < 0) {
		pr_err("failed: cell-index rc %d", rc);
		goto FREE_SENSOR_DATA;
	}

	/* Validate pdev->id */
	if (pdev->id >= MAX_CAMERAS) {
		pr_err("failed: invalid pdev->id %d", pdev->id);
		rc = -EINVAL;
		goto FREE_SENSOR_DATA;
	}

	/* Check whether g_sctrl is already filled for this pdev id */
	if (g_sctrl[pdev->id]) {
		pr_err("failed: sctrl already filled for id %d", pdev->id);
		rc = -EINVAL;
		goto FREE_SENSOR_DATA;
	}

	/* Read subdev info */
	rc = msm_sensor_get_sub_module_index(of_node, &sensordata->sensor_info);
	if (rc < 0) {
		pr_err("failed");
		goto FREE_SENSOR_DATA;
	}

	/* Read vreg information */
	rc = msm_camera_get_dt_vreg_data(of_node,
					 &sensordata->power_info.cam_vreg,
					 &sensordata->power_info.num_vreg);
	if (rc < 0) {
		pr_err("failed: msm_camera_get_dt_vreg_data rc %d", rc);
		goto FREE_SUB_MODULE_DATA;
	}

	/* Read gpio information */
	rc = msm_sensor_driver_get_gpio_data(sensordata, of_node);
	if (rc < 0) {
		pr_err("failed: msm_sensor_driver_get_gpio_data rc %d", rc);
		goto FREE_VREG_DATA;
	}

	/* Get CCI master */
	rc = of_property_read_u32(of_node, "qcom,cci-master",
				  &s_ctrl->cci_i2c_master);
	CDBG("qcom,cci-master %d, rc %d", s_ctrl->cci_i2c_master, rc);
	if (rc < 0) {
		/* Set default master 0 */
		s_ctrl->cci_i2c_master = MASTER_0;
		rc = 0;
	}

	/* Get mount angle */
	rc = of_property_read_u32(of_node, "qcom,mount-angle",
				  &sensordata->sensor_info->sensor_mount_angle);
	CDBG("%s qcom,mount-angle %d, rc %d\n", __func__,
	     sensordata->sensor_info->sensor_mount_angle, rc);
	if (rc < 0) {
		sensordata->sensor_info->is_mount_angle_valid = 0;
		sensordata->sensor_info->sensor_mount_angle = 0;
		rc = 0;
	} else {
		sensordata->sensor_info->is_mount_angle_valid = 1;
	}

	/* Get vdd-cx regulator */
	/*Optional property, don't return error if absent */
	of_property_read_string(of_node, "qcom,vdd-cx-name",
				&sensordata->misc_regulator);
	CDBG("qcom,misc_regulator %s", sensordata->misc_regulator);

	return rc;

 FREE_VREG_DATA:
	kfree(sensordata->power_info.cam_vreg);
 FREE_SUB_MODULE_DATA:
	kfree(sensordata->sensor_info);
 FREE_SENSOR_DATA:
	kfree(sensordata);
	return rc;
}

static int32_t msm_sensor_driver_parse(struct platform_device *pdev)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl = NULL;

	CDBG("Enter\n");
	/* Validate input parameters */
	if (!pdev || !pdev->dev.of_node) {
		pr_err("failed: invalid params");
		return -EINVAL;
	}

	/* Create sensor control structure */
	s_ctrl = kzalloc(sizeof(*s_ctrl), GFP_KERNEL);
	if (!s_ctrl) {
		pr_err("failed: no memory s_ctrl %pK", s_ctrl);
		return -ENOMEM;
	}

	/* Fill platform device */
	s_ctrl->pdev = pdev;

	/* Allocate memory for sensor_i2c_client */
	s_ctrl->sensor_i2c_client = kzalloc(sizeof(*s_ctrl->sensor_i2c_client),
					    GFP_KERNEL);
	if (!s_ctrl->sensor_i2c_client) {
		pr_err("failed: no memory sensor_i2c_client %pK",
		       s_ctrl->sensor_i2c_client);
		goto FREE_SCTRL;
	}

	/* Allocate memory for mutex */
	s_ctrl->msm_sensor_mutex = kzalloc(sizeof(*s_ctrl->msm_sensor_mutex),
					   GFP_KERNEL);
	if (!s_ctrl->msm_sensor_mutex) {
		pr_err("failed: no memory msm_sensor_mutex %pK",
		       s_ctrl->msm_sensor_mutex);
		goto FREE_SENSOR_I2C_CLIENT;
	}

	/* Parse dt information and store in sensor control structure */
	rc = msm_sensor_driver_get_dt_data(s_ctrl, pdev);
	if (rc < 0) {
		pr_err("failed: rc %d", rc);
		goto FREE_MUTEX;
	}

	/* Fill device in power info */
	s_ctrl->sensordata->power_info.dev = &pdev->dev;

	/* Initialize mutex */
	mutex_init(s_ctrl->msm_sensor_mutex);

	/* Initilize v4l2 subdev info */
	s_ctrl->sensor_v4l2_subdev_info = msm_sensor_driver_subdev_info;
	s_ctrl->sensor_v4l2_subdev_info_size =
		ARRAY_SIZE(msm_sensor_driver_subdev_info);

	/* Initialize default parameters */
	rc = msm_sensor_init_default_params(s_ctrl);
	if (rc < 0) {
		pr_err("failed: msm_sensor_init_default_params rc %d", rc);
		goto FREE_DT_DATA;
	}

	/* Store sensor control structure in static database */
	g_sctrl[pdev->id] = s_ctrl;
	pr_warn("g_sctrl[%d] %pK\n", pdev->id, g_sctrl[pdev->id]);

	return rc;

 FREE_DT_DATA:
	kfree(s_ctrl->sensordata->power_info.gpio_conf->gpio_num_info);
	kfree(s_ctrl->sensordata->power_info.gpio_conf->cam_gpio_req_tbl);
	kfree(s_ctrl->sensordata->power_info.gpio_conf);
	kfree(s_ctrl->sensordata->power_info.cam_vreg);
	kfree(s_ctrl->sensordata);
 FREE_MUTEX:
	kfree(s_ctrl->msm_sensor_mutex);
 FREE_SENSOR_I2C_CLIENT:
	kfree(s_ctrl->sensor_i2c_client);
 FREE_SCTRL:
	kfree(s_ctrl);
	return rc;
}

static int __init msm_sensor_driver_init(void)
{
	int32_t rc = 0;

	pr_warn("%s : Enter", __func__);
	rc = platform_driver_probe(&msm_sensor_platform_driver,
				   msm_sensor_driver_parse);
	if (!rc)
		pr_warn("probe success");
	return rc;
}


static void __exit msm_sensor_driver_exit(void)
{
	CDBG("Enter");
	return;
}

module_init(msm_sensor_driver_init);
module_exit(msm_sensor_driver_exit);
MODULE_DESCRIPTION("msm_sensor_driver");
MODULE_LICENSE("GPL v2");
