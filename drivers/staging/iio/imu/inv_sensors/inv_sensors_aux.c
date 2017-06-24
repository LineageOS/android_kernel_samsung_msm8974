#include "inv_sensors_sm.h"
#include "inv_sensors_common.h"


/* AKM definitions */
#define REG_AKM_ID               0x00
#define REG_AKM_INFO             0x01
#define REG_AKM_STATUS           0x02
#define REG_AKM_MEASURE_DATA     0x03
#define REG_AKM_MODE             0x0A
#define REG_AKM_ST_CTRL          0x0C
#define REG_AKM_SENSITIVITY      0x10
#define REG_AKM8963_CNTL1        0x0A

#define DATA_AKM_ID              0x48
#define DATA_AKM_MODE_PD	 0x00
#define DATA_AKM_MODE_SM	 0x01
#define DATA_AKM_MODE_ST	 0x08
#define DATA_AKM_MODE_FR	 0x0F
#define DATA_AK09911_MODE_FR     0x1F
#define DATA_AKM_SELF_TEST       0x40
#define DATA_AKM_DRDY            0x01
#define DATA_AKM8963_BIT         0x10
#define DATA_AKM_STAT_MASK       0x0C

#define DATA_AKM8975_SCALE       (9830 * (1L << 15))
#define DATA_AKM8972_SCALE       (19661 * (1L << 15))
#define DATA_AKM8963_SCALE0      (19661 * (1L << 15))
#define DATA_AKM8963_SCALE1      (4915 * (1L << 15))
#define DATA_AK09911_SCALE       (19661 * (1L << 15))
#define DATA_MLX_SCALE           (4915 * (1L << 15))
#define DATA_MLX_SCALE_EMPIRICAL (26214 * (1L << 15))

#define DATA_AKM8963_SCALE_SHIFT      4
#define DATA_AKM_99_BYTES_DMP  10
#define DATA_AKM_89_BYTES_DMP  9
#define DATA_AKM_MIN_READ_TIME            (9 * NSEC_PER_MSEC)

#define DEF_ST_COMPASS_WAIT_MIN     (10 * 1000)
#define DEF_ST_COMPASS_WAIT_MAX     (15 * 1000)
#define DEF_ST_COMPASS_TRY_TIMES    10
#define DEF_ST_COMPASS_8963_SHIFT   2
#define NUM_BYTES_COMPASS_SLAVE (8 + 1)


/*
	mpu65xx's reg
*/
#define REG_I2C_SLV1_DO         (0x64)
#define REG_I2C_MST_CTRL        (0x24)
#define REG_I2C_SLV0_ADDR       (0x25)
#define REG_I2C_SLV0_REG        (0x26)
#define REG_I2C_SLV0_CTRL       (0x27)
#define REG_I2C_SLV1_ADDR       (0x28)
#define REG_I2C_SLV1_REG        (0x29)
#define REG_I2C_SLV1_CTRL       (0x2A)

#define REG_I2C_SLV4_CTRL       (0x34)
#define REG_INT_PIN_CFG		(0x37)
#define REG_I2C_MST_DELAY_CTRL  (0x67)


#define BIT_WAIT_FOR_ES         (0x40)

#define BIT_BYPASS_EN           0x2
#define BIT_I2C_READ    (0x80)
#define BIT_SLV_EN              (0x80)

#define BIT_SLV0_DLY_EN			0x01
#define BIT_SLV1_DLY_EN			0x02


struct aux_ctrl_t {
	u16 i2c_addr;
	struct invsens_i2c_t *i2c_handle;

	struct invsens_sm_ctrl_t sm_ctrl;

	int8_t orientation[9];
	bool is_secondary;
	u8 master_i2c_addr;
	u8 master_int_config;
	int compass_scale;
	u8 compass_sens[3];

	u8 aux_id;
	u32 enable_mask;
};



static int aux_setup_hw_akm8963(struct aux_ctrl_t *dctl)
{
	int res = 0;
	u8 regs[4];
	u8 sens, mode, cmd;

	INV_DBG_FUNC_NAME;

	if (dctl->is_secondary) {
		regs[0] = dctl->master_int_config | BIT_BYPASS_EN;

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_INT_PIN_CFG, regs[0], res,
				      error_case);
	}


	i2c_read(dctl, REG_AKM_ID, 1, regs, res, error_case);

	if (regs[0] != DATA_AKM_ID) {
		pr_info("Not akm device (%x)\n", regs[0]);
		res = -ENXIO;
		goto error_case;
	}

	pr_info("[inv_sensors] found AKM device(8963)\n");

	mode = REG_AKM_MODE;
	sens = REG_AKM_SENSITIVITY;
	cmd = DATA_AKM_MODE_FR;

	i2c_write_single(dctl, mode, cmd, res, error_case);

	/* Wait at least 200us */
	udelay(200);

	/*read sensitivity */
	i2c_read(dctl, sens, 3, dctl->compass_sens, res, error_case);

	/* revert to power down mode */
	i2c_write_single(dctl, mode, DATA_AKM_MODE_PD, res, error_case);

	pr_debug("senx=%d, seny=%d, senz=%d\n",
		 dctl->compass_sens[0],
		 dctl->compass_sens[1], dctl->compass_sens[2]);


	if (dctl->is_secondary) {
		regs[0] = dctl->master_int_config;
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_INT_PIN_CFG, regs[0], res,
				      error_case);

		/*mpu configuration for slave compass */
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_I2C_MST_CTRL, BIT_WAIT_FOR_ES,
				      res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_I2C_SLV1_ADDR, dctl->i2c_addr,
				      res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_I2C_SLV1_REG, mode, res,
				      error_case);

		regs[0] =
		    DATA_AKM_MODE_SM | (1 << DATA_AKM8963_SCALE_SHIFT);
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_I2C_SLV1_DO, regs[0], res,
				      error_case);

		regs[0] = BIT_SLV0_DLY_EN | BIT_SLV1_DLY_EN;
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
				      REG_I2C_MST_DELAY_CTRL, regs[0], res,
				      error_case);
	}


	return SM_SUCCESS;

error_case:
	INVSENS_LOGE("%s : error = %d\n", __func__, res);
	return res;
}



static int aux_enable_akm8963(struct aux_ctrl_t *dctl, u32 func_mask,
	bool enable)
{
	int res = SM_SUCCESS;
	u8 reg;
	u8 delay = 0;
	bool compass_enable = false;
	u32 mask;

	INV_DBG_FUNC_NAME;

	INVSENS_LOGD("%s : enable = %d\n", __func__, enable);

	if (!dctl->is_secondary)
		return -SM_EUNSUPPORT;

	mask = func_mask & (1 << INV_FUNC_COMPASS);

	if (mask)
		compass_enable = true;

	if (compass_enable) {
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV0_ADDR, BIT_I2C_READ | dctl->i2c_addr,
			res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV0_REG, REG_AKM_INFO, res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV0_CTRL,
			BIT_SLV_EN | NUM_BYTES_COMPASS_SLAVE,
			res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV1_CTRL, BIT_SLV_EN | 1, res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_MST_DELAY_CTRL, (u8) delay, res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV4_CTRL, 0, res, error_case);

		reg =
			DATA_AKM_MODE_SM | (1 << DATA_AKM8963_SCALE_SHIFT);
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV1_DO, reg, res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_MST_DELAY_CTRL,
			BIT_SLV0_DLY_EN | BIT_SLV1_DLY_EN, res,
			error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV4_CTRL, 0x1f, res, error_case);

	} else {
		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV0_CTRL, 0, res, error_case);

		i2c_write_single_base(dctl, dctl->master_i2c_addr,
			REG_I2C_SLV1_CTRL, 0, res, error_case);
	}

	dctl->enable_mask = mask;

	return SM_SUCCESS;

error_case:
	INVSENS_LOGE("%s : error = %d\n", __func__, res);
	return res;
}



static int aux_if_init(struct invsens_driver_t *drv)
{
	int res = 0;
	struct aux_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (dctl->aux_id == INVSENS_AID_AKM8963)
		res = aux_setup_hw_akm8963(dctl);

	if (res)
		goto error_case;

	dctl->enable_mask = 0;

	return SM_SUCCESS;

error_case:
	return res;
}

static int aux_if_enable(struct invsens_driver_t *drv,
		       u32 func_mask, bool enable,
		       struct invsens_sm_ctrl_t *sm_ctrl)
{
	int res = 0;
	struct aux_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (sm_ctrl)
		memcpy(&dctl->sm_ctrl, sm_ctrl, sizeof(dctl->sm_ctrl));

	if (dctl->aux_id == INVSENS_AID_AKM8963)
		res = aux_enable_akm8963(dctl, func_mask, enable);

	if (res)
		INVSENS_LOGE("akm8963 enable error = %d\n", res);

	if (dctl->enable_mask)
		drv->is_activated = true;
	else
		drv->is_activated = false;

	return res;
}



static int aux_if_set_delay(struct invsens_driver_t *drv,
			  u32 func_mask, long delay)
{
	int res = 0;

	INV_DBG_FUNC_NAME;


	return res;
}

static int aux_if_read(struct invsens_driver_t *drv,
		     struct invsens_data_list_t *buffer)
{
	int res = 0;

	INV_DBG_FUNC_NAME;

	return res;
}


static int aux_if_ioctl(struct invsens_driver_t *drv, u32 cmd,
		     long lparam, void *vparam)
{

	INV_DBG_FUNC_NAME;


	return SM_IOCTL_NOTHANDLED;
}

int aux_if_term(struct invsens_driver_t * drv)
{
	struct aux_ctrl_t *dctl = NULL;
	INV_DBG_FUNC_NAME;
	dctl = INVSENS_DRV_USER_DATA(drv);
	kzfree(dctl);
	drv->user_data = NULL;
	return SM_SUCCESS;
}

/*
	describe the specification of device driver
*/

static struct invsens_driver_t aux_handler = {
	.driver_id = INV_DRIVER_AUX,
	.driver_layer = INV_DRIVER_LAYER_NATIVE,
	.func_mask = (1 << INV_FUNC_COMPASS),

	.init = aux_if_init,
	.enable = aux_if_enable,
	.set_delay = aux_if_set_delay,
	.read = aux_if_read,
	.ioctl = aux_if_ioctl,
	.terminate = aux_if_term,
};

int invsens_load_aux(struct invsens_driver_t **driver,
	struct invsens_board_cfg_t *board_cfg)
{
	int res = 0;
	struct aux_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	if (!driver) {
		res = -EINVAL;
		goto error_case;
	}

	dctl = kzalloc(sizeof(struct aux_ctrl_t), GFP_KERNEL);
	if (!dctl) {
		res = -ENOMEM;
		goto error_case;
	}

	if (!board_cfg->platform_data) {
		res = -EINVAL;
		goto error_case;
	}

	dctl->i2c_addr = board_cfg->platform_data->compass.i2c_addr;
	dctl->i2c_handle = board_cfg->i2c_handle;

	memcpy(dctl->orientation,
	       board_cfg->platform_data->compass.orientation,
	       sizeof(dctl->orientation));
	dctl->master_int_config = board_cfg->platform_data->int_config;

	if (board_cfg->platform_data->compass.aux_id != INVSENS_AID_NONE) {
		dctl->is_secondary = true;
		dctl->master_i2c_addr = board_cfg->i2c_addr;
		dctl->aux_id = board_cfg->platform_data->compass.aux_id;
	} else
		dctl->is_secondary = false;

	aux_handler.user_data = (void *) dctl;

	*driver = &aux_handler;

	return 0;

 error_case:
	if (dctl != NULL)
		kfree(dctl);

	return res;
}
