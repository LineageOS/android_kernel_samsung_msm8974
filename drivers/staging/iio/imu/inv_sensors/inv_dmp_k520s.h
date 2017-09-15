#ifndef _INV_DMP_K520S_H_
#define _INV_DMP_K520S_H_

#include "inv_dmp_base.h"

#define DMP_FEAT_KEYMAP_HELPER(feat, type) \
	{ INV_DFEAT_##feat, CFG_OUT_##type, \
		CFG_##type##_ODR, ODR_CNTR_##type}

#define DMP_FEAT_KEYMAP_HELPER1(feat, type, odr) \
	{ INV_DFEAT_##feat, CFG_OUT_##type, \
	CFG_##odr##_ODR, ODR_CNTR_##type}


int invdmp_load_module(struct invdmp_driver_t *drv,
	struct invsens_board_cfg_t *board_cfg, void *dbase_data);


#endif	/*_INV_DMP_K520S_H_ */
