#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/sensor/inv_sensors.h>

#include "../../iio.h"
#include "../../buffer.h"
#include "../../kfifo_buf.h"
#include "../../sysfs.h"

#include "inv_sensors_sm.h"
#include "inv_sensors_common.h"

#include "inv_dmp_k520s.h"

#define FIRMWARE_FILENAME "dfk520rc13.bin"
#define DMP_START_ADDRESS 0x400

#define mem_w(dctl, addr, len, data) \
	invdmp_write_memory(addr, len, data, dctl->dbase_data)

#define mem_r(dctl, addr, len, data) \
	invdmp_read_memory(addr, len, data, dctl->dbase_data)

#define AXIS_NUM 3
#define AXIS_ADC_BYTE 2
#define QUAT_AXIS_BYTE 4
#define QUAT_AXIS_NUM 3
#define PACKET_SIZE_PER_SENSOR (AXIS_NUM * AXIS_ADC_BYTE)
#define PACKET_SIZE_PER_QUAT (QUAT_AXIS_BYTE*QUAT_AXIS_NUM)
#define PACKET_SIZE_PER_STED (4)
#define PACKET_SIZE_PER_SMD  (0)

/*
	protocol definition between kenel and chip
*/
#define ACCEL_HDR   0x4000
#define GYRO_HDR    0x2000
#define CPASS_HDR   0x1000
#define PQUAT9_HDR  0x0A00
#define QUAT9_HDR   0x0900
#define LPQUAT_HDR  0x0800
#define QUAT6_HDR   0x0400
#define PQUAT_HDR   0x0200
#define STEPDET_HDR 0x0100
#define PRESS_HDR   0x8000
#define PEDO_HDR    0x9000
#define SMD_HDR	  0xA000
#define ACCEL_HDR_EXT   0x4001
#define GYRO_HDR_EXT   0x2001
#define CPASS_HDR_EXT   0x1001
#define LPQUAT_HDR_EXT 0x0801

struct dmp_feat_mem_t {
	int feat;
	u16 send_addr;
	u16 odr_mem;
	u16 odr_cntr_mem;
};

#define CFG_OUT_PRESS               (1823)
#define CFG_PED_ENABLE              (1936)
#define CFG_OUT_GYRO                (1755)
#define CFG_PEDSTEP_DET             (2417)
#define OUT_GYRO_DAT                (1764)
#define CFG_FIFO_INT                (1934)
#define OUT_CPASS_DAT               (1798)
#define OUT_ACCL_DAT                (1730)
#define FCFG_1                      (1078)
#define FCFG_3                      (1103)
#define FCFG_2                      (1082)
#define CFG_OUT_CPASS               (1789)
#define FCFG_7                      (1089)
#define CFG_OUT_3QUAT               (1617)
#define OUT_PRESS_DAT               (1832)
#define OUT_3QUAT_DAT               (1627)
#define OUT_PQUAT_DAT               (1696)
#define CFG_OUT_6QUAT               (1652)
#define CFG_PED_INT                 (2406)
#define CFG_OUT_ACCL                (1721)
#define CFG_OUT_STEPDET             (1587)
#define OUT_6QUAT_DAT               (1662)
#define CFG_OUT_PQUAT               (1687)

#define D_0_22                  (22+512)



#define D_SMD_ENABLE            (18 * 16)
#define D_SMD_MOT_THLD          (21 * 16 + 8)
#define D_SMD_DELAY_THLD        (21 * 16 + 4)
#define D_SMD_DELAY2_THLD       (21 * 16 + 12)
#define D_SMD_EXE_STATE         (22 * 16)
#define D_SMD_DELAY_CNTR        (21 * 16)


#define D_PEDSTD_BP_B           (768 + 0x1C)
#define D_PEDSTD_BP_A4          (768 + 0x40)
#define D_PEDSTD_BP_A3          (768 + 0x44)
#define D_PEDSTD_BP_A2          (768 + 0x48)
#define D_PEDSTD_BP_A1          (768 + 0x4C)
#define D_PEDSTD_SB             (768 + 0x28)
#define D_PEDSTD_SB_TIME        (768 + 0x2C)
#define D_PEDSTD_PEAKTHRSH      (768 + 0x98)
#define D_PEDSTD_TIML           (768 + 0x2A)
#define D_PEDSTD_TIMH           (768 + 0x2E)
#define D_PEDSTD_PEAK           (768 + 0X94)
#define D_PEDSTD_STEPCTR        (768 + 0x60)
#define D_PEDSTD_STEPCTR2       (58 * 16 + 8)
#define D_PEDSTD_TIMECTR        (964)
#define D_PEDSTD_DECI           (768 + 0xA0)
#define D_PEDSTD_SB2			(60 * 16 + 14)
#define D_STPDET_TIMESTAMP      (28 * 16 + 8)
#define D_PEDSTD_DRIVE_STATE    (58)

#define D_HOST_NO_MOT           (976)
#define D_ACCEL_BIAS            (660)

#define D_BM_BATCH_CNTR         (27 * 16 + 4)
#define D_BM_BATCH_THLD         (27 * 16 + 12)
#define D_BM_ENABLE             (28 * 16 + 6)
#define D_BM_NUMWORD_TOFILL     (28 * 16 + 4)

#define D_SO_DATA               (4 * 16 + 2)

#define D_P_HW_ID               (22 * 16 + 10)
#define D_P_INIT                (22 * 16 + 2)

#define D_DMP_RUN_CNTR          (24*16)

#define D_EXT_GYRO_BIAS_X       (61 * 16)
#define D_EXT_GYRO_BIAS_Y       (61 * 16 + 4)
#define D_EXT_GYRO_BIAS_Z       (61 * 16 + 8)



#define CFG_3QUAT_ODR                (45*16+12)
#define CFG_6QUAT_ODR                (45*16+10)
#define CFG_PQUAT6_ODR                (45*16+14)
#define CFG_ACCL_ODR                (46*16+8)
#define CFG_GYRO_ODR                (46*16+12)
#define CFG_CPASS_ODR                (46*16+10)
#define CFG_PRESS_ODR                (46*16+14)
#define CFG_9QUAT_ODR                (42*16+8)
#define CFG_PQUAT9_ODR                (42*16+12)

#define ODR_CNTR_3QUAT           (45*16+4)
#define ODR_CNTR_6QUAT           (45*16+2)
#define ODR_CNTR_PQUAT           (45*16+6)
#define ODR_CNTR_ACCL           (46*16)
#define ODR_CNTR_GYRO           (46*16+4)
#define ODR_CNTR_CPASS           (46*16+2)
#define ODR_CNTR_PRESS           (46*16+6)
#define ODR_CNTR_9QUAT           (42*16)
#define ODR_CNTR_PQUAT9           (42*16+4)



#define CFG_PQUAT_ODR CFG_PQUAT6_ODR


static struct dmp_feat_mem_t dmp_feat_output[] = {
	DMP_FEAT_KEYMAP_HELPER(ACCEL, ACCL),
	DMP_FEAT_KEYMAP_HELPER(GYRO, GYRO),
	DMP_FEAT_KEYMAP_HELPER(COMPASS, CPASS),
	DMP_FEAT_KEYMAP_HELPER(PRESSURE, PRESS),
	DMP_FEAT_KEYMAP_HELPER(3AXIS_QUAT, 3QUAT),
	DMP_FEAT_KEYMAP_HELPER(6AXIS_QUAT, 6QUAT),
	DMP_FEAT_KEYMAP_HELPER(PED_QUAT, PQUAT),
	{INV_DFEAT_STEP_IND, CFG_PEDSTEP_DET, 0, 0},
	{INV_DFEAT_STEP_DETECTOR, CFG_OUT_STEPDET, 0, 0},
};


#define FEAT_KEY_CONFIG_NUM \
	(sizeof(dmp_feat_output) / sizeof(dmp_feat_output[0]))


struct dmp_func_match_t {
	int func;
	int feat_mask;
};

static struct dmp_func_match_t dmp_match_table[] = {
	{INV_FUNC_ACCEL, (1 << INV_DFEAT_ACCEL)},
	{INV_FUNC_GYRO, (1 << INV_DFEAT_GYRO)},
	{INV_FUNC_COMPASS, (1 << INV_DFEAT_COMPASS)},
	{INV_FUNC_GAMING_ROTATION_VECTOR,
		((1 << INV_DFEAT_ACCEL) | (1 << INV_DFEAT_GYRO)
		| (1 << INV_DFEAT_3AXIS_QUAT))},
	{INV_FUNC_SIGNIFICANT_MOTION_DETECT, (1 << INV_DFEAT_SMD)},
	{INV_FUNC_STEP_DETECT,
		(1 << INV_DFEAT_STEP_DETECTOR) | (1 << INV_DFEAT_PEDO)},
	{INV_FUNC_STEP_COUNT,
		(1 << INV_DFEAT_STEP_DETECTOR) | (1 << INV_DFEAT_PEDO)},
	{INV_FUNC_SCREEN_ORIENTATION, (1 << INV_DFEAT_SCREEN_ORIENTATION)},
	{INV_FUNC_ROTATION_VECTOR,
		((1 << INV_DFEAT_ACCEL) | (1 << INV_DFEAT_GYRO)
		| (1 << INV_DFEAT_COMPASS) | (1 << INV_DFEAT_3AXIS_QUAT))},
	{INV_FUNC_GEOMAG_ROTATION_VECTOR,
		(1 << INV_DFEAT_ACCEL) | (1 << INV_DFEAT_COMPASS)},
	{INV_FUNC_BATCH, 0},
	{INV_FUNC_FLUSH, 0},
};


#define FUNC_MATCH_TABLE_NUM \
	(sizeof(dmp_match_table) / sizeof(dmp_match_table[0]))



struct dmp_dctl_t {
	struct {
		bool is_enabled;
		int (*enable) (struct dmp_dctl_t *dctl, bool en);
		int (*set_rate) (struct dmp_dctl_t *dctl, long delay);
	} feat_set[INV_DFEAT_MAX];
	long feat_delay[INV_DFEAT_MAX];
	struct invsens_sm_ctrl_t sm_ctrl;
	struct invsens_platform_data_t *platform_data;

	void *dbase_data;
	u32 enabled_mask;
};

static int dmp_write_2bytes(struct dmp_dctl_t *dctl, u16 addr, int data)
{
	u8 d[2];

	if (data < 0 || data > USHRT_MAX)
		return -EINVAL;
	d[0] = (u8) ((data >> 8) & 0xff);
	d[1] = (u8) (data & 0xff);
	return mem_w(dctl, addr, ARRAY_SIZE(d), d);
}

static u16 dmp_row_2_scale(const s8 *row)
{
	u16 b;

	if (row[0] > 0)
		b = 0;

	else if (row[0] < 0)
		b = 4;

	else if (row[1] > 0)
		b = 1;

	else if (row[1] < 0)
		b = 5;

	else if (row[2] > 0)
		b = 2;

	else if (row[2] < 0)
		b = 6;

	else
		b = 7;
	return b;
}

static u16 dmp_orientation_matrix_to_scaler(const signed char *mtx)
{
	u16 scalar;

	scalar = dmp_row_2_scale(mtx);
	scalar |= dmp_row_2_scale(mtx + 3) << 3;
	scalar |= dmp_row_2_scale(mtx + 6) << 6;
	return scalar;
}

static int dmp_gyro_cal(struct dmp_dctl_t *dctl,
			const signed char orientation[9])
{
	int res = 0;
	u8 tmpD = 0x4c;
	u8 tmpE = 0xcd;
	u8 tmpF = 0x6c;
	int inv_gyro_orient;
	u8 regs[3];

	INV_DBG_FUNC_NAME;

	inv_gyro_orient = dmp_orientation_matrix_to_scaler(orientation);
	if ((inv_gyro_orient & 3) == 0)
		regs[0] = tmpD;

	else if ((inv_gyro_orient & 3) == 1)
		regs[0] = tmpE;

	else if ((inv_gyro_orient & 3) == 2)
		regs[0] = tmpF;
	if ((inv_gyro_orient & 0x18) == 0)
		regs[1] = tmpD;

	else if ((inv_gyro_orient & 0x18) == 0x8)
		regs[1] = tmpE;

	else if ((inv_gyro_orient & 0x18) == 0x10)
		regs[1] = tmpF;
	if ((inv_gyro_orient & 0xc0) == 0)
		regs[2] = tmpD;

	else if ((inv_gyro_orient & 0xc0) == 0x40)
		regs[2] = tmpE;

	else if ((inv_gyro_orient & 0xc0) == 0x80)
		regs[2] = tmpF;
	res = mem_w(dctl, FCFG_1, ARRAY_SIZE(regs), regs);
	if (res)
		goto error_case;
	if (inv_gyro_orient & 4)
		regs[0] = 0x36 | 1;

	else
		regs[0] = 0x36;
	if (inv_gyro_orient & 0x20)
		regs[1] = 0x56 | 1;

	else
		regs[1] = 0x56;
	if (inv_gyro_orient & 0x100)
		regs[2] = 0x76 | 1;

	else
		regs[2] = 0x76;
	res = mem_w(dctl, FCFG_3, ARRAY_SIZE(regs), regs);
	if (res)
		goto error_case;
	return 0;

error_case:
	return res;
}

static int dmp_accel_cal(struct dmp_dctl_t *dctl,
			const signed char orientation[9])
{
	int inv_accel_orient;
	int res;
	u8 regs[3];
	const u8 tmp[3] = { 0x0c, 0xc9, 0x2c };
	INV_DBG_FUNC_NAME;
	inv_accel_orient = dmp_orientation_matrix_to_scaler(orientation);
	regs[0] = tmp[inv_accel_orient & 3];
	regs[1] = tmp[(inv_accel_orient >> 3) & 3];
	regs[2] = tmp[(inv_accel_orient >> 6) & 3];
	res = mem_w(dctl, FCFG_2, ARRAY_SIZE(regs), regs);
	if (res)
		return res;
	regs[0] = 0x26;
	regs[1] = 0x46;
	regs[2] = 0x66;
	if (inv_accel_orient & 4)
		regs[0] |= 1;
	if (inv_accel_orient & 0x20)
		regs[1] |= 1;
	if (inv_accel_orient & 0x100)
		regs[2] |= 1;
	res = mem_w(dctl, FCFG_7, ARRAY_SIZE(regs), regs);
	return res;
}

static int dmp_set_rate(struct dmp_dctl_t *dctl, int feat, int rate)
{
	int i, v, res = 0;
	u16 k = 0, k_ct = 0;

	INV_DBG_FUNC_NAME;
	for (i = 0; i < FEAT_KEY_CONFIG_NUM; i++)
		if (dmp_feat_output[i].feat == feat) {
			k = dmp_feat_output[i].odr_mem;
			k_ct = dmp_feat_output[i].odr_cntr_mem;
			break;
		}
	if (k == 0 && k_ct == 0)
		goto error_case;
	v = INVDMP_DEFAULT_FREQ / rate - 1;
	res = dmp_write_2bytes(dctl, k, v);
	if (res)
		return res;
	res = dmp_write_2bytes(dctl, k_ct, 0);
	return SM_SUCCESS;

error_case:
	return res;
}

static int dmp_send_data(struct dmp_dctl_t *dctl, int feat, bool on)
{
	u8 rn[] = {
		0xa3, 0xa3
	};
	u8 rf[] = {
		0xf4, 0x12
	};
	int res, i = 0;
	u8 *r;
	u16 addr = -1;

	INV_DBG_FUNC_NAME;
	if (on)
		r = rn;

	else
		r = rf;
	for (i = 0; i < FEAT_KEY_CONFIG_NUM; i++)
		if (dmp_feat_output[i].feat == feat) {
			addr = dmp_feat_output[i].send_addr;
			break;
		}
	if (addr < 0)
		return -SM_EINVAL;
	res = mem_w(dctl, addr, ARRAY_SIZE(rf), r);
	return res;
}

static int dmp_enable_accel(struct dmp_dctl_t *dctl, bool en)
{
	INV_DBG_FUNC_NAME;
	return dmp_send_data(dctl, INV_DFEAT_ACCEL, en);
}

static int dmp_enable_gyro(struct dmp_dctl_t *dctl, bool en)
{
	INV_DBG_FUNC_NAME;
	return dmp_send_data(dctl, INV_DFEAT_GYRO, en);
}

static int dmp_enable_compass(struct dmp_dctl_t *dctl, bool en)
{
	INV_DBG_FUNC_NAME;
	return dmp_send_data(dctl, INV_DFEAT_COMPASS, en);
}

static int dmp_enable_grv(struct dmp_dctl_t *dctl, bool en)
{
	INV_DBG_FUNC_NAME;
	return dmp_send_data(dctl, INV_DFEAT_6AXIS_QUAT, en);
}

static int dmp_enable_lpq(struct dmp_dctl_t *dctl, bool en)
{
	INV_DBG_FUNC_NAME;
	return dmp_send_data(dctl, INV_DFEAT_3AXIS_QUAT, en);
}

static int dmp_enable_smd(struct dmp_dctl_t *dctl, bool en)
{
	int v = 0;

	INV_DBG_FUNC_NAME;
	if (en)
		v = 1;
	return dmp_write_2bytes(dctl, D_SMD_ENABLE, v);
}

static int dmp_enable_step_detect(struct dmp_dctl_t *dctl, bool en)
{
	INV_DBG_FUNC_NAME;
	return dmp_send_data(dctl, INV_DFEAT_STEP_DETECTOR, en);
}

static int dmp_enable_pedometer(struct dmp_dctl_t *dctl, bool en)
{
	u8 d[1];

	INV_DBG_FUNC_NAME;
	if (en)
		d[0] = 0xf1;
	else
		d[0] = 0xff;
	return mem_w(dctl, CFG_PED_ENABLE, ARRAY_SIZE(d), d);
}

static int dmp_set_accel_delay(struct dmp_dctl_t *dctl, long delay)
{
	int rate;

	INV_DBG_FUNC_NAME;
	rate = (int) (1000000000L / delay);
	return dmp_set_rate(dctl, INV_DFEAT_ACCEL, rate);
}

static int dmp_set_gyro_delay(struct dmp_dctl_t *dctl, long delay)
{
	int rate;

	INV_DBG_FUNC_NAME;
	rate = (int) (1000000000L / delay);
	return dmp_set_rate(dctl, INV_DFEAT_GYRO, rate);
}

static int dmp_set_compass_delay(struct dmp_dctl_t *dctl, long delay)
{
	int rate;

	INV_DBG_FUNC_NAME;
	rate = (int) (1000000000L / delay);
	return dmp_set_rate(dctl, INV_DFEAT_COMPASS, rate);
}

static int dmp_set_grv_delay(struct dmp_dctl_t *dctl, long delay)
{
	int rate;

	INV_DBG_FUNC_NAME;
	rate = (int) (1000000000L / delay);
	return dmp_set_rate(dctl, INV_DFEAT_6AXIS_QUAT, rate);
}

static int dmp_set_lpq_delay(struct dmp_dctl_t *dctl, long delay)
{
	int rate;

	INV_DBG_FUNC_NAME;
	rate = (int) (1000000000L / delay);
	return dmp_set_rate(dctl, INV_DFEAT_3AXIS_QUAT, rate);
}

static int dmp_get_pedometer_steps(struct dmp_dctl_t *dctl,
	u32 *steps)
{
	u8 d[4];
	int res;

	INV_DBG_FUNC_NAME;
	res = mem_r(dctl, D_PEDSTD_STEPCTR, 4, d);
	*steps = (u32) be32_to_cpup((__be32 *) (d));
	return res;
}

static int dmp_trans_func2feat(struct dmp_dctl_t *dctl,
				u32 func_mask, u32 *feat_mask)
{
	int res = 0;
	int i = 0, j = 0;
	u32 mask = 0;

	INV_DBG_FUNC_NAME;
	for (i = 0; i < INV_FUNC_NUM; i++)
		if (func_mask & (1 << i))
			for (j = 0; j < FUNC_MATCH_TABLE_NUM; j++)
				if (dmp_match_table[j].func == i)
					mask |= dmp_match_table[j].feat_mask;

	/*exclude compass if it isn't */
	if (!dctl->sm_ctrl.has_compass)
		mask &= ~(1 << INV_DFEAT_COMPASS);
	*feat_mask = mask;
	return res;
}

static void dmp_update_feat_delays(struct dmp_dctl_t *dctl, u32 feat_mask)
{
	long accel_delay = 200000000L;
	long gyro_delay = 200000000L;
	long compass_delay = 200000000L;
	long lpq_delay = 200000000L;
	bool is_lpq_enabled = false;
	u32 func = 0;

	/*-----------------------------------------*/
	/* decide if LPQ is used and it's delay        */
	/*-----------------------------------------*/
	/* 1. check GRV first */
	func = INV_FUNC_GAMING_ROTATION_VECTOR;
	if (dctl->sm_ctrl.enable_mask & (1 << func)) {
		lpq_delay = INV_MIN(lpq_delay, dctl->sm_ctrl.delays[func]);
		is_lpq_enabled = true;
	}

	/* 2. check RV first */
	func = INV_FUNC_ROTATION_VECTOR;
	if (dctl->sm_ctrl.enable_mask & (1 << func)) {
		lpq_delay = INV_MIN(lpq_delay, dctl->sm_ctrl.delays[func]);
		is_lpq_enabled = true;
	}
	INVSENS_LOGD("%s : lpq_delay = %ld\n", __func__, lpq_delay);

	/*-----------------------------------------*/
	/* decide the delays of accel, gyro and compass        */
	/*-----------------------------------------*/
	/* 1. check accel */
	func = INV_FUNC_ACCEL;
	if (dctl->sm_ctrl.enable_mask & (1 << func)) {
		accel_delay = dctl->sm_ctrl.delays[func];
		if (is_lpq_enabled)
			lpq_delay = INV_MIN(lpq_delay, accel_delay);
	}

	/* 2. check gyro */
	func = INV_FUNC_GYRO;
	if (dctl->sm_ctrl.enable_mask & (1 << func)) {
		gyro_delay = dctl->sm_ctrl.delays[func];
		if (is_lpq_enabled)
			lpq_delay = INV_MIN(lpq_delay, gyro_delay);
	}

	/* 3. check compass */
	func = INV_FUNC_COMPASS;
	if (dctl->sm_ctrl.enable_mask & (1 << func)) {
		compass_delay = dctl->sm_ctrl.delays[func];
		if (is_lpq_enabled)
			lpq_delay = INV_MIN(lpq_delay, compass_delay);
	}

	/*-----------------------------------------*/
	/* decide the delays of accel/compass by GMRV    */
	/*-----------------------------------------*/
	func = INV_FUNC_GEOMAG_ROTATION_VECTOR;
	if (dctl->sm_ctrl.enable_mask & (1 << func)) {
		long min_delay = dctl->sm_ctrl.delays[func];

		min_delay = INV_MIN(min_delay, accel_delay);
		if (dctl->sm_ctrl.has_compass)
			min_delay = INV_MIN(min_delay, compass_delay);
		accel_delay = min_delay;
		if (dctl->sm_ctrl.has_compass)
			compass_delay = min_delay;
		if (is_lpq_enabled)
			lpq_delay = INV_MIN(lpq_delay, min_delay);
	}
	if (is_lpq_enabled) {
		accel_delay = lpq_delay;
		gyro_delay = lpq_delay;
		compass_delay = lpq_delay;
	}
	dctl->feat_delay[INV_DFEAT_ACCEL] = accel_delay;
	dctl->feat_delay[INV_DFEAT_GYRO] = gyro_delay;
	dctl->feat_delay[INV_DFEAT_COMPASS] = compass_delay;
	dctl->feat_delay[INV_DFEAT_3AXIS_QUAT] = lpq_delay;
	dctl->feat_delay[INV_DFEAT_6AXIS_QUAT] = lpq_delay;
}

static long dmp_get_delay(struct dmp_dctl_t *dctl, int feat)
{
	long delay = dctl->feat_delay[feat];

	INVSENS_LOGD("%s : delay = %ld\n", __func__, delay);
	return delay;
}

static int dmp_enable_features(struct dmp_dctl_t *dctl, u32 feat_mask)
{
	int res = 0;
	int i = 0;
	bool enable;

	INV_DBG_FUNC_NAME;
	INVSENS_LOGD("%s : feat_mask = 0x%x\n", __func__, feat_mask);
	dmp_update_feat_delays(dctl, feat_mask);

	for (i = 0; i < INV_DFEAT_MAX; i++) {
		if (dctl->feat_set[i].enable) {
			if (feat_mask & (1 << i))
				enable = true;
			else
				enable = false;

			res = dctl->feat_set[i].enable(dctl, enable);
			dctl->feat_set[i].is_enabled = enable;

			if (!res && dctl->feat_set[i].set_rate && enable)
				dctl->feat_set[i].set_rate(dctl,
					dmp_get_delay(dctl, i));
		}
	}
	return res;
}

static int dmp_suspend_features(struct dmp_dctl_t *dctl)
{
	int res = 0;
	int i = 0;

	INV_DBG_FUNC_NAME;

	if (!dctl->sm_ctrl.is_dmp_on)
		return SM_SUCCESS;

	for (i = 0; i < INV_DFEAT_MAX; i++) {
		if (dctl->feat_set[i].enable) {
			switch (i) {
			case INV_DFEAT_ACCEL:
			case INV_DFEAT_GYRO:
			case INV_DFEAT_COMPASS:
			case INV_DFEAT_PRESSURE:
			case INV_DFEAT_3AXIS_QUAT:
			case INV_DFEAT_6AXIS_QUAT:
			case INV_DFEAT_PEDO:
				res = dctl->feat_set[i].enable(dctl,
					false);
				break;
			}
		}
	}
	return res;
}

static int dmp_resume_features(struct dmp_dctl_t *dctl)
{
	int res = 0;
	int i = 0;

	INV_DBG_FUNC_NAME;

	if (!dctl->sm_ctrl.is_dmp_on)
		return SM_SUCCESS;

	for (i = 0; i < INV_DFEAT_MAX; i++) {
		switch (i) {
		case INV_DFEAT_ACCEL:
		case INV_DFEAT_GYRO:
		case INV_DFEAT_COMPASS:
		case INV_DFEAT_PRESSURE:
		case INV_DFEAT_3AXIS_QUAT:
		case INV_DFEAT_6AXIS_QUAT:
		case INV_DFEAT_PEDO:
			if (dctl->feat_set[i].enable) {
				res = dctl->feat_set[i].enable(dctl,
					dctl->feat_set[i].is_enabled);
				if (!res && dctl->feat_set[i].set_rate)
					dctl->feat_set[i].set_rate(dctl,
						dmp_get_delay(dctl, i));
			}
			break;
		}
	}

	return res;
}



static int dmp_apply_mpu_orientation(struct dmp_dctl_t *dctl,
				     const u8 src[], u8 dest[])
{
	short tmp[AXIS_NUM];
	int oriented[AXIS_NUM];

	int i = 0;

	tmp[0] = ((short) src[0] << 8) + src[1];
	tmp[1] = ((short) src[2] << 8) + src[3];
	tmp[2] = ((short) src[4] << 8) + src[5];

	for (i = 0; i < AXIS_NUM; i++) {
		oriented[i] = tmp[0]*dctl->platform_data->orientation[i*3 + 0]
		    + tmp[1]*dctl->platform_data->orientation[i*3 + 1]
		    + tmp[2]*dctl->platform_data->orientation[i*3 + 2];

		/*handle overflow */
		if (oriented[i] > 32767)
			oriented[i] = 32767;
	}

	for (i = 0; i < AXIS_NUM; i++)
		tmp[i] = (short) oriented[i];

	memcpy((void *) dest, (void *) tmp, sizeof(tmp));

	return 0;
}

/*
	temporary
*/
static int dmp_appy_compass_orientation(struct dmp_dctl_t *dctl,
					       const u8 src[], u8 dest[])
{
	short tmp[AXIS_NUM];
	int oriented[AXIS_NUM];

	int i = 0;

	/*little endian */
	tmp[0] = ((short) src[1] << 8) + src[0];
	tmp[1] = ((short) src[3] << 8) + src[2];
	tmp[2] = ((short) src[5] << 8) + src[4];

	for (i = 0; i < AXIS_NUM; i++) {
		oriented[i] =
		    tmp[0]*dctl->platform_data->compass.orientation[i*3 + 0]
		    + tmp[1]*dctl->platform_data->compass.orientation[i*3 + 1]
		    + tmp[2]*dctl->platform_data->compass.orientation[i*3 + 2];

		/*handle overflow */
		if (oriented[i] > 32767)
			oriented[i] = 32767;
	}

	for (i = 0; i < AXIS_NUM; i++)
		tmp[i] = (short) oriented[i];

	memcpy((void *) dest, (void *) tmp, sizeof(tmp));

	return 0;
}


static int dmp_parse_fifo(struct dmp_dctl_t *dctl,
			      int len, const char *data,
			      struct invsens_data_list_t *buffer)
{
	u8 *p = (u8 *) data;
	u16 header;
	int res = SM_SUCCESS;

	INV_DBG_FUNC_NAME;

	while (p && p < (u8 *) (data + len)
		 && (buffer->count < INVSENS_DATA_ITEM_MAX-1)) {
		/*find header */
		header = (u16) p[0] << 8 | p[1];

		p += sizeof(u16);

		switch (header) {
		case ACCEL_HDR:
		case ACCEL_HDR_EXT:
			{
				struct invsens_data_t *d =
				    &buffer->items[buffer->count++];
				d->hdr = INV_DATA_HDR_ACCEL;
				d->length = PACKET_SIZE_PER_SENSOR;
				dmp_apply_mpu_orientation(dctl, p,
							  &d->data[0]);
				INVSENS_LOGD("ACCEL_HDR\n");
				p += d->length;
			}
			break;
		case GYRO_HDR:
		case GYRO_HDR_EXT:
			{
				struct invsens_data_t *d =
				    &buffer->items[buffer->count++];
				d->hdr = INV_DATA_HDR_GYRO;
				d->length = PACKET_SIZE_PER_SENSOR;
				dmp_apply_mpu_orientation(dctl, p,
							  &d->data[0]);
				INVSENS_LOGD("GYRO_HDR\n");
				p += d->length;
			}
			break;
		case CPASS_HDR:
		case CPASS_HDR_EXT:
			{
				struct invsens_data_t *d =
				    &buffer->items[buffer->count++];
				d->hdr = INV_DATA_HDR_COMPASS;
				d->length = PACKET_SIZE_PER_SENSOR;
				dmp_appy_compass_orientation(dctl,
					p, &d->data[0]);
				INVSENS_LOGD("CPASS_HDR\n");
				p += d->length;

			}
			break;
		case QUAT6_HDR:
			{
				struct invsens_data_t *d =
				    &buffer->items[buffer->count++];
				int len = QUAT_AXIS_BYTE;
				p += sizeof(u16);
				d->hdr = INV_DATA_HDR_6AXIS_QUAT;
				d->length = PACKET_SIZE_PER_QUAT;
				invsens_swap_array(p, &d->data[0], len);
				invsens_swap_array(p + len, &d->data[len], len);
				invsens_swap_array(p + len*2, &d->data[len*2],
					len);
				INVSENS_LOGD("QUAT6_HDR\n");
				p += d->length;
			}
			break;
		case LPQUAT_HDR:
		case LPQUAT_HDR_EXT:
			{
				struct invsens_data_t *d =
				    &buffer->items[buffer->count++];
				int len = QUAT_AXIS_BYTE;
				p += sizeof(u16);
				d->hdr = INV_DATA_HDR_LPQUAT;
				d->length = PACKET_SIZE_PER_QUAT;
				invsens_swap_array(p, &d->data[0], len);
				invsens_swap_array(p + len, &d->data[len], len);
				invsens_swap_array(p + len * 2,
						   &d->data[len * 2], len);
				INVSENS_LOGD("LPQUAT_HDR\n");
				p += d->length;
			}
			break;
		case STEPDET_HDR:
			{
				struct invsens_data_t *d =
				    &buffer->items[buffer->count++];
				p += sizeof(u16);
				d->hdr = INV_DATA_HDR_STEPDET;
				d->length = PACKET_SIZE_PER_STED;
				memcpy(&d->data[0], p, d->length);
				INVSENS_LOGD("STEPDET_HDR\n");

				p += d->length;
			}
			break;
		default:
			INVSENS_LOGD("unknow header = 0x%02x\n", header);
			buffer->request_fifo_reset = true;
			return -1;
		}
	};

	if (buffer->count > INVSENS_DATA_ITEM_MAX - 1) {
		buffer->request_fifo_reset = true;
		INVSENS_LOGE("Reset fifo\n");
	}

	return res;
}

static int dmp_k520s_enable(struct invdmp_driver_t *drv,
				u32 func_mask, bool enabled, void *sm_ctrl)
{
	int res = 0;
	struct dmp_dctl_t *dctl = NULL;
	u32 enable_mask = 0;
	u32 feat_mask = 0;

	INV_DBG_FUNC_NAME;
	dctl = INVSENS_DRV_USER_DATA(drv);
	if (sm_ctrl)
		memcpy(&dctl->sm_ctrl, sm_ctrl,
			sizeof(struct invsens_sm_ctrl_t));
	enable_mask = dctl->sm_ctrl.enable_mask;
	INVSENS_LOGD("%s : enable_mask = 0x%x\n", __func__, enable_mask);
	res = dmp_trans_func2feat(dctl, enable_mask, &feat_mask);
	if (res)
		goto error_case;

	res = dmp_enable_features(dctl, feat_mask);
	if (res)
		goto error_case;
	dctl->enabled_mask = enable_mask;

 error_case:
	return res;
}

static int dmp_k520s_terminate(struct invdmp_driver_t *drv)
{
	int res = 0;
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;
	dctl = INVSENS_DRV_USER_DATA(drv);
	kzfree(dctl);
	INVSENS_DRV_USER_DATA(drv) = NULL;
	return res;
}

static int dmp_k520s_read(struct invdmp_driver_t *drv,
				struct invsens_data_list_t *buffer)
{
	int res = SM_SUCCESS;
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	if (dctl->sm_ctrl.is_dmp_on
		&& dctl->sm_ctrl.enable_mask
		&& (buffer->is_fifo_data_copied)) {
		dmp_parse_fifo(dctl, buffer->fifo_data_length,
			buffer->copy_buffer, buffer);
	}

	if (dctl->sm_ctrl.enable_mask & (1 << INV_FUNC_STEP_COUNT)) {
		int i = 0;

		for (i = 0; i < buffer->count; i++) {
			struct invsens_data_t *p = &buffer->items[i];

			if (p->hdr == INV_DATA_HDR_STEPDET) {

				/*read step counter */
				u32 current_steps = 0;

				dmp_get_pedometer_steps(dctl, &current_steps);

				INVSENS_LOGD("DMP_PEDO ; %d\n", current_steps);
				if (current_steps > 0) {
					p = &buffer->items[buffer->count++];
					p->hdr = INV_DATA_HDR_PEDO;
					p->length = sizeof(u32);
					memcpy(p->data, (void *)&current_steps,
						p->length);
				}
				break;
			}
		}
	}
	return res;
}

static int dmp_k520s_suspend(struct invdmp_driver_t *drv)
{
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;
	dctl = INVSENS_DRV_USER_DATA(drv);

	return dmp_suspend_features(dctl);
}

static int dmp_k520s_resume(struct invdmp_driver_t *drv)
{
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;
	dctl = INVSENS_DRV_USER_DATA(drv);

	return dmp_resume_features(dctl);
}


static int dmp_init_config(struct dmp_dctl_t *dctl)
{
	int res = SM_SUCCESS;

	memset((void *) dctl->feat_set, 0x0, sizeof(dctl->feat_set));
	dctl->feat_set[INV_DFEAT_ACCEL].enable = dmp_enable_accel;
	dctl->feat_set[INV_DFEAT_ACCEL].set_rate = dmp_set_accel_delay;
	dctl->feat_set[INV_DFEAT_GYRO].enable = dmp_enable_gyro;
	dctl->feat_set[INV_DFEAT_GYRO].set_rate = dmp_set_gyro_delay;
	dctl->feat_set[INV_DFEAT_COMPASS].enable = dmp_enable_compass;
	dctl->feat_set[INV_DFEAT_COMPASS].set_rate = dmp_set_compass_delay;
	dctl->feat_set[INV_DFEAT_3AXIS_QUAT].enable = dmp_enable_lpq;
	dctl->feat_set[INV_DFEAT_3AXIS_QUAT].set_rate = dmp_set_lpq_delay;
	dctl->feat_set[INV_DFEAT_6AXIS_QUAT].enable = dmp_enable_grv;
	dctl->feat_set[INV_DFEAT_6AXIS_QUAT].set_rate = dmp_set_grv_delay;
	dctl->feat_set[INV_DFEAT_PED_QUAT].enable = NULL;
	dctl->feat_set[INV_DFEAT_PED_QUAT].set_rate = NULL;
	dctl->feat_set[INV_DFEAT_STEP_IND].enable = NULL;
	dctl->feat_set[INV_DFEAT_STEP_IND].set_rate = NULL;
	dctl->feat_set[INV_DFEAT_STEP_DETECTOR].enable =
		dmp_enable_step_detect;
	dctl->feat_set[INV_DFEAT_STEP_DETECTOR].set_rate = NULL;
	dctl->feat_set[INV_DFEAT_SCREEN_ORIENTATION].enable = NULL;
	dctl->feat_set[INV_DFEAT_SCREEN_ORIENTATION].set_rate = NULL;
	dctl->feat_set[INV_DFEAT_SMD].enable = dmp_enable_smd;
	dctl->feat_set[INV_DFEAT_SMD].set_rate = NULL;
	dctl->feat_set[INV_DFEAT_PEDO].enable = dmp_enable_pedometer;
	dctl->feat_set[INV_DFEAT_PEDO].set_rate = NULL;
	return res;
}

static int dmp_set_cal(struct dmp_dctl_t *dctl,
					   const signed char orientation[9])
{
	int res = 0;

	res = dmp_gyro_cal(dctl, orientation);
	if (res)
		goto error_case;
	res = dmp_accel_cal(dctl, orientation);
	if (res)
		goto error_case;
	dctl->enabled_mask = 0;

 error_case:
	return res;
}

static int dmp_k520s_init(struct invdmp_driver_t *drv,
			const signed char orientation[9])
{
	int res = 0;
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;
	dctl = INVSENS_DRV_USER_DATA(drv);
	if (dctl) {
		res = invdmp_load_firmware(FIRMWARE_FILENAME, 0x20,
				DMP_START_ADDRESS, dctl->dbase_data);
		if (res) {
			INVSENS_LOGE("firmware error = %d !!!!\n", res);
			goto error_case;
		}

		res = dmp_init_config(dctl);
		if (res)
			INVSENS_LOGI("%s configuration error\n", drv->version);
		res = dmp_set_cal(dctl, orientation);
		if (!res)
			INVSENS_LOGI("%s is ready\n", drv->version);
		return SM_SUCCESS;
	}

 error_case:
	return res;
}

static int dmp_k520s_ioctl(struct invdmp_driver_t *drv, u32 cmd,
			     long lparam, void *vparm)
{
	struct invsens_ioctl_param_t *data = vparm;
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	switch (cmd) {
	case INV_IOCTL_SET_GYRO_DMP_BIAS:
		{
			int v, i;
			u8 d[4];

			 for (i = 0; i < 3; i++) {
				v = (int)data->wdgb.bias[i];
				invsens_swap_array((u8 *)&v, d, sizeof(d));
				mem_w(dctl, D_EXT_GYRO_BIAS_X + i * 4,
					ARRAY_SIZE(d), d);
			 }
		}
		return SM_IOCTL_HANDLED;

	case INV_IOCTL_SET_ACCEL_DMP_BIAS:
		{

		}
		return SM_IOCTL_HANDLED;

	}

	return SM_IOCTL_NOTHANDLED;
}


struct invdmp_driver_t dmp_k520s_driver = {
	.version = {"DMPv2 K520SRC13"},
	.init = dmp_k520s_init,
	.enable_func = dmp_k520s_enable,
	.read = dmp_k520s_read,
	.suspend = dmp_k520s_suspend,
	.resume = dmp_k520s_resume,
	.terminate = dmp_k520s_terminate,
	.ioctl = dmp_k520s_ioctl,
};

int invdmp_load_module(struct invdmp_driver_t *drv,
	struct invsens_board_cfg_t *board_cfg, void *dbase_data)
{
	int res = 0;
	struct dmp_dctl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;
	if (!drv) {
		res = -EINVAL;
		goto error_case;
	}
	memcpy(drv, &dmp_k520s_driver, sizeof(struct invdmp_driver_t));

	dctl = kzalloc(sizeof(struct dmp_dctl_t), GFP_KERNEL);
	if (!dctl) {
		res = -ENOMEM;
		goto error_case;
	}
	dctl->dbase_data = dbase_data;
	dctl->platform_data = board_cfg->platform_data;

	drv->user_data = (void *) dctl;
	return 0;

 error_case:
	return res;
}
