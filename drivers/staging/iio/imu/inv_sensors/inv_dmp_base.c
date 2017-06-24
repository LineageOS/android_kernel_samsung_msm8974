
#include "inv_sensors_sm.h"
#include "inv_sensors_common.h"
#include "inv_dmp_base.h"
#include "inv_dmp_fw.h"



struct dmp_ctrl_t {
	u16 i2c_addr;
	char cipher_key[DMP_CIPHER_KEY_LEN];
	struct invsens_i2c_t *i2c_handle;

	struct invdmp_driver_t dmp_drv;
	struct invsens_sm_ctrl_t sm_ctrl;
	struct invdmp_fw_data_t firmware_cfg;
	struct invsens_board_cfg_t *board_cfg;

	signed char orientation[9];

	unsigned int enabled_mask;
	bool is_batch_enabled;
};


int invdmp_load_module(struct invdmp_driver_t *drv,
	struct invsens_board_cfg_t *board_cfg, void *dbase_data);

static int dmp_set_delay(struct invsens_driver_t *drv, u32 func_mask,
			 long delay, bool update_drvier)
{
	int res = 0;

	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	return res;
}



static int dmp_enable_dmp(struct invsens_driver_t *drv, u32 func_mask,
			  bool enable)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (dctl->dmp_drv.enable_func) {
		res = dctl->dmp_drv.enable_func(&dctl->dmp_drv,
						func_mask, enable,
						&dctl->sm_ctrl);
		if (res)
			goto error_case;
	}
	return 0;

 error_case:
	return res;
}


static int dmp_enable_sensor(struct invsens_driver_t *drv, u32 func_mask,
			     bool enable)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;

	u32 enable_mask;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	enable_mask = dctl->sm_ctrl.enable_mask;

	INVSENS_LOGD("[inv]%s : func_mask = 0x%x, enable_mask = 0x%x\n",
		     __func__, func_mask, enable_mask);

	if (dctl->sm_ctrl.is_dmp_on) {
		res = dmp_enable_dmp(drv, func_mask, enable);
		if (res)
			goto error_case;
	}

	dctl->enabled_mask = enable_mask;
	INVSENS_LOGD(" %s : enable_mask = 0x%x\n", __func__, enable_mask);

error_case:
	return res;
}




static int dmp_if_init(struct invsens_driver_t *drv)
{
	int res = 0;

	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	return res;
}


static int dmp_if_sync(struct invsens_driver_t *drv, const char *key)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	if (!key) {
		res = -EINVAL;
		goto error_case;
	}

	dctl = INVSENS_DRV_USER_DATA(drv);

	memcpy(dctl->cipher_key, key, DMP_CIPHER_KEY_LEN);

	res = invdmp_load_module(&dctl->dmp_drv,
		dctl->board_cfg, (void *) dctl);
	if (res)
		goto error_case;

	if (dctl->dmp_drv.init)
		dctl->dmp_drv.init(&dctl->dmp_drv, dctl->orientation);

	return 0;

 error_case:
	return res;
}





static int dmp_if_enable(struct invsens_driver_t *drv,
			      u32 func_mask, bool enable,
			      struct invsens_sm_ctrl_t *sm_ctrl)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (sm_ctrl) {
		memcpy(&dctl->sm_ctrl, sm_ctrl, sizeof(dctl->sm_ctrl));
		dctl->sm_ctrl.enable_mask &= drv->func_mask;
	}

	res = dmp_enable_sensor(drv, func_mask, enable);

	if (dctl->enabled_mask)
		drv->is_activated = true;
	else
		drv->is_activated = false;

	return res;
}

static int dmp_if_delay(struct invsens_driver_t *drv, u32 func_mask,
			     long delay)
{
	return dmp_set_delay(drv, func_mask, delay, true);
}


static int dmp_if_batch(struct invsens_driver_t *drv,
			     u32 func_mask, long delay, int timeout)
{
	int res = 0;


	INV_DBG_FUNC_NAME;

	return res;
}


static int dmp_if_flush(struct invsens_driver_t *drv, u32 func_mask)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;


	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	return res;
}


static int dmp_if_read(struct invsens_driver_t *drv,
			    struct invsens_data_list_t *buffer)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (dctl->dmp_drv.read)
		res = dctl->dmp_drv.read(&dctl->dmp_drv, buffer);

	return res;
}


static int dmp_if_terminate(struct invsens_driver_t *drv)
{
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	kzfree(dctl->firmware_cfg.key_map);
	kzfree(dctl);
	drv->user_data = NULL;

	return SM_SUCCESS;
}

static int dmp_if_suspend(struct invsens_driver_t *drv)
{
	int res = SM_SUCCESS;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (dctl->dmp_drv.suspend)
			res = dctl->dmp_drv.suspend(&dctl->dmp_drv);

	return res;
}

static int dmp_if_resume(struct invsens_driver_t *drv)
{
	int res = SM_SUCCESS;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);
	if (dctl->dmp_drv.resume)
			res = dctl->dmp_drv.resume(&dctl->dmp_drv);

	return res;
}


static int dmp_if_ioctl(struct invsens_driver_t *drv, u32 cmd,
			     long lparam, void *vparam)
{
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (dctl->dmp_drv.ioctl)
		return dctl->dmp_drv.ioctl(&dctl->dmp_drv, cmd, lparam, vparam);

	return SM_IOCTL_NOTHANDLED;
}




static struct invsens_driver_t dmp_handler = {
	.driver_id = INV_DRIVER_DMP,
	.driver_layer = INV_DRIVER_LAYER_PLATFORM,

	.func_mask =
	    (1 << INV_FUNC_ACCEL) | (1 << INV_FUNC_GYRO) |
	    (1 << INV_FUNC_COMPASS) | (1 <<  INV_FUNC_GAMING_ROTATION_VECTOR) |
	    (1 << INV_FUNC_SIGNIFICANT_MOTION_DETECT) |
	    (1 << INV_FUNC_STEP_DETECT) | (1 << INV_FUNC_STEP_COUNT) |
	    (1 << INV_FUNC_SCREEN_ORIENTATION)  |
	    (1 << INV_FUNC_ROTATION_VECTOR) |
	    (1 << INV_FUNC_BATCH) | (1 << INV_FUNC_FLUSH) |
	    (1 << INV_FUNC_GEOMAG_ROTATION_VECTOR),

	.init = dmp_if_init,
	.sync = dmp_if_sync,
	.enable = dmp_if_enable,
	.set_delay = dmp_if_delay,
	.batch = dmp_if_batch,
	.flush = dmp_if_flush,
	.read = dmp_if_read,
	.suspend = dmp_if_suspend,
	.resume = dmp_if_resume,
	.terminate = dmp_if_terminate,

	.ioctl = dmp_if_ioctl,
};


int invsens_load_dmp(struct invsens_driver_t **driver,
	struct invsens_board_cfg_t *board_cfg)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	if (!board_cfg || !board_cfg->platform_data) {
		res = -EINVAL;
		goto error_case;
	}

	dctl = kzalloc(sizeof(struct dmp_ctrl_t), GFP_KERNEL);
	if (!dctl) {
		res = -ENOMEM;
		goto error_case;
	}

	dctl->i2c_handle = board_cfg->i2c_handle;
	dctl->i2c_addr = board_cfg->i2c_addr;
	dctl->board_cfg = board_cfg;

	memcpy(dctl->orientation,
	       board_cfg->platform_data->orientation,
	       sizeof(dctl->orientation));

	dmp_handler.user_data = dctl;

	*driver = &dmp_handler;

error_case:
	return res;
}


/*************
	basic apis
*************/


int invdmp_write_memory(u16 addr, u16 length, const u8 *data,
			void *dbase_data)
{
	struct dmp_ctrl_t *dctl = dbase_data;

	INV_DBG_FUNC_NAME;

	return inv_i2c_mem_write(dctl->i2c_handle, dctl->i2c_addr,
				 addr, length, (u8 *) data);
}

int invdmp_read_memory(u16 addr, u16 length, u8 *data, void *dbase_data)
{
	struct dmp_ctrl_t *dctl = dbase_data;

	INV_DBG_FUNC_NAME;

	return inv_i2c_mem_read(dctl->i2c_handle, dctl->i2c_addr,
				addr, length, data);
}

int invdmp_load_firmware(const char *filename, int offset, int start_addr,
	void *dbase_data)
{
	int res = 0;
	struct dmp_ctrl_t *dctl = dbase_data;

	res = invdmp_download_fw(filename, offset, dctl->i2c_handle,
				 dctl->i2c_addr, dctl->cipher_key,
				 &dctl->firmware_cfg);
	if (res)
		goto error_case;

	dctl->sm_ctrl.start_addr = start_addr;

	return 0;

error_case:
	return res;
}
