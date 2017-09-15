#include <linux/err.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>

#include <linux/i2c.h>
#include <linux/sensor/inv_sensors.h>
#include "inv_sensors_dts.h"

int inv_sensors_power_on(struct invsens_platform_data_t *pdata)
{
	int err ;
	err = regulator_enable(pdata->vdd);
	pr_info("%s:mpu_power_on %d\n", "", err);
	return err ;
}

int inv_sensors_power_off(struct invsens_platform_data_t *pdata)
{
	int err ;

	err = regulator_disable(pdata->vdd);
	pr_info("%s:mpu_power_off %d\n", "", err);

	return err;
}

int inv_sensors_orientation_matrix(struct device *dev, s8 *orient)
{
	int rc, i;
	struct device_node *np = dev->of_node;
	u32 orientation[9];

	rc = of_property_read_u32_array(np, "inven,orientation", orientation, 9);
	if (rc) {
		dev_err(dev, "Unable to read orientation\n");
		return rc;
	}
	for (i = 0 ; i < 9 ; i++)
		orient[i] = ((s8)orientation[i]) - 1;


	return 0;
}

int inv_sensors_parse_secondary_orientation(struct device *dev,
							s8 *orient)
{
	int rc, i;
	struct device_node *np = dev->of_node;
	u32 temp_val, temp_val2;

	for (i = 0; i < 9; i++)
		orient[i] = 0;

	/* parsing axis x orientation matrix*/
	rc = of_property_read_u32(np, "inven,secondary_axis_map_x", &temp_val);
	if (rc) {
		dev_err(dev, "Unable to read secondary axis_map_x\n");
		return rc;
	}
	rc = of_property_read_u32(np, "inven,secondary_negate_x", &temp_val2);
	if (rc) {
		dev_err(dev, "Unable to read secondary negate_x\n");
		return rc;
	}
	if (temp_val2)
		orient[temp_val] = -1;
	else
		orient[temp_val] = 1;

	/* parsing axis y orientation matrix*/
	rc = of_property_read_u32(np, "inven,secondary_axis_map_y", &temp_val);
	if (rc) {
		dev_err(dev, "Unable to read secondary axis_map_y\n");
		return rc;
	}
	rc = of_property_read_u32(np, "inven,secondary_negate_y", &temp_val2);
	if (rc) {
		dev_err(dev, "Unable to read secondary negate_y\n");
		return rc;
	}
	if (temp_val2)
		orient[3 + temp_val] = -1;
	else
		orient[3 + temp_val] = 1;

	/* parsing axis z orientation matrix*/
	rc = of_property_read_u32(np, "inven,secondary_axis_map_z", &temp_val);
	if (rc) {
		dev_err(dev, "Unable to read secondary axis_map_z\n");
		return rc;
	}
	rc = of_property_read_u32(np, "inven,secondary_negate_z", &temp_val2);
	if (rc) {
		dev_err(dev, "Unable to read secondary negate_z\n");
		return rc;
	}
	if (temp_val2)
		orient[6 + temp_val] = -1;
	else
		orient[6 + temp_val] = 1;

	return 0;
}

int inv_sensors_parse_secondary(struct device *dev,
	struct invsens_platform_data_t *pdata)
{
	int rc;
	struct device_node *np = dev->of_node;
	u32 temp_val;
	const char *name;

	if (of_property_read_string(np, "inven,secondary_type", &name)) {
		dev_err(dev, "Missing secondary type.\n");
		pdata->compass.aux_id = INVSENS_AID_NONE;
		return 0;
	}

	if (of_property_read_string(np, "inven,secondary_name", &name)) {
		dev_err(dev, "Missing secondary name.\n");
		return -EINVAL;
	}

	if (!strcmp(name, "ak8963"))
		pdata->compass.aux_id = INVSENS_AID_AKM8963;
	else if (!strcmp(name, "ak8975"))
		pdata->compass.aux_id = INVSENS_AID_AKM8975;
	else if (!strcmp(name, "ak09911"))
		pdata->compass.aux_id = INVSENS_AID_AKM09911;
	else
		return -EINVAL;

	rc = of_property_read_u32(np, "inven,secondary_reg", &temp_val);
	if (rc) {
		dev_err(dev, "Unable to read secondary register\n");
		return rc;
	}
	pdata->compass.i2c_addr = temp_val;
	rc = inv_sensors_parse_secondary_orientation(dev,
						pdata->compass.orientation);

	return rc;
}

int inv_sensors_parse_aux(struct device *dev,
	struct invsens_platform_data_t *pdata)
{
#if 0
	int rc;
	struct device_node *np = dev->of_node;
	u32 temp_val;
	const char *name;

	if (of_property_read_string(np, "inven,aux_type", &name)) {
		dev_err(dev, "Missing aux type.\n");
		return -EINVAL;
	}
	if (!strcmp(name, "pressure")) {
		pdata->aux_slave_type = SECONDARY_SLAVE_TYPE_PRESSURE;
	} else if (!strcmp(name, "none")) {
		pdata->aux_slave_type = SECONDARY_SLAVE_TYPE_NONE;
		return 0;
	} else {
		return -EINVAL;
	}

	if (of_property_read_string(np, "inven,aux_name", &name)) {
		dev_err(dev, "Missing aux name.\n");
		return -EINVAL;
	}
	if (!strcmp(name, "bmp280"))
		pdata->aux_slave_id = PRESSURE_ID_BMP280;
	else
		return -EINVAL;

	rc = of_property_read_u32(np, "inven,aux_reg", &temp_val);
	if (rc) {
		dev_err(dev, "Unable to read aux register\n");
		return rc;
	}
	pdata->aux_i2c_addr = temp_val;
#endif
	return 0;
}

int inv_sensors_parse_dt(struct device *dev,
	struct invsens_platform_data_t *pdata)
{
	int rc;
	pr_info("%s: Invensense MPU parse_dt started.\n", __func__);

	rc = inv_sensors_orientation_matrix(dev, pdata->orientation);
	if (rc)
		return rc;
	pr_info("%s: Invensense MPU parse_dt started 0.\n", __func__);
	rc = inv_sensors_parse_secondary(dev, pdata);
	if (rc)
		return rc;
	pr_info("%s: Invensense MPU parse_dt started 1.\n", __func__);
	inv_sensors_parse_aux(dev, pdata);

	pdata->vdd = regulator_get(dev, "inven,vdd");
	if (IS_ERR(pdata->vdd)) {
		rc = PTR_ERR(pdata->vdd);
		dev_err(dev,
			"Regulator get failed vdd-supply rc=%d\n", rc);
		return rc;
	}

	pr_info("%s: Invensense MPU parse_dt started 3.\n", __func__);
	pdata->power_on = inv_sensors_power_on;
	pdata->power_off = inv_sensors_power_off;
	pr_debug("Invensense MPU parse_dt complete.\n");
	pr_info("%s: Invensense MPU parse_dt complete.\n", __func__);
	return rc;
}
