#include "inv_sensors_common.h"

#define MPU65XX_SMPLRT_DIV		0x19
#define MPU65XX_CONFIG			0x1A
#define MPU65XX_GYRO_CONFIG		0x1B
#define MPU65XX_ACCEL_CONFIG		0x1C
#define MPU65XX_ACCEL_CONFIG2		0x1D
#define MPU65XX_LP_ACCEL_ODR		0x1E
#define MPU65XX_WOM_THRESH		0x1F
#define MPU65XX_FIFO_EN			0x23
#define MPU65XX_INT_PIN_CFG		0x37
#define MPU65XX_INT_ENABLE		0x38
#define MPU65XX_DMP_INT_STATUS		0x39
#define MPU65XX_INT_STATUS		0x3A
#define MPU65XX_ACCEL_XOUT_H		0x3B
#define MPU65XX_TEMP_OUT_H		0x41
#define MPU65XX_GYRO_XOUT_H		0x43
#define MPU65XX_ACCEL_INTEL_CTRL	0x69
#define MPU65XX_USER_CTRL		0x6A
#define MPU65XX_PWR_MGMT_1		0x6B
#define MPU65XX_PWR_MGMT_2		0x6C
#define MPU65XX_PRGM_STRT_ADDRH		0x70
#define MPU65XX_FIFO_COUNTH		0x72
#define MPU65XX_FIFO_R_W		0x74
#define MPU65XX_WHOAMI			0x75


/*------------------------------
	MPU65XX_CONFIG
--------------------------------*/
#define DLPF_CFG_250HZ		0x00
#define DLPF_CFG_184HZ		0x01
#define DLPF_CFG_98HZ		0x02
#define DLPF_CFG_41HZ		0x03
#define DLPF_CFG_20HZ		0x04
#define DLPF_CFG_10HZ		0x05
#define DLPF_CFG_5HZ		0x06
#define DLPF_CFG_3600HZ		0x07


/*------------------------------
	MPU65XX_GYRO_CONFIG
--------------------------------*/
#define GFSR_250DPS		(0 << 3)
#define GFSR_500DPS		(1 << 3)
#define GFSR_1000DPS		(2 << 3)
#define GFSR_2000DPS		(3 << 3)

/*------------------------------
	MPU65XX_ACCEL_CONFIG
--------------------------------*/
#define AFSR_2G		(0 << 3)
#define AFSR_4G		(1 << 3)
#define AFSR_8G		(2 << 3)
#define AFSR_16G	(3 << 3)


/*------------------------------
	MPU65XX_ACCEL_CONFIG2
--------------------------------*/
#define A_DLPF_CFG_460HZ	0x00
#define A_DLPF_CFG_184HZ	0x01
#define A_DLPF_CFG_92HZ		0x02
#define A_DLPF_CFG_41HZ		0x03
#define A_DLPF_CFG_20HZ		0x04
#define A_DLPF_CFG_10HZ		0x05
#define A_DLPF_CFG_5HZ		0x06
/*#define A_DLPF_CFG_460HZ	0x07*/
#define BIT_FIFO_SIZE_1K	0x40
#define BIT_ACCEL_FCHOICE_B	0x08


/*------------------------------
	MPU65XX_LP_ACCEL_ODR
--------------------------------*/
#define LPA_CLK_P24HZ	0x0
#define LPA_CLK_P49HZ	0x1
#define LPA_CLK_P98HZ	0x2
#define LPA_CLK_1P95HZ	0x3
#define LPA_CLK_3P91HZ	0x4
#define LPA_CLK_7P81HZ	0x5
#define LPA_CLK_15P63HZ	0x6
#define LPA_CLK_31P25HZ	0x7
#define LPA_CLK_62P50HZ	0x8
#define LPA_CLK_125HZ	0x9
#define LPA_CLK_250HZ	0xa
#define LPA_CLK_500HZ	0xb


/*------------------------------
	MPU65XX_PWR_MGMT_1
--------------------------------*/
#define BIT_H_RESET		(1<<7)
#define BIT_SLEEP		(1<<6)
#define BIT_CYCLE		(1<<5)
#define BIT_GYRO_STANDBY	(1<<4)
#define BIT_PD_PTAT		(1<<3)
#define BIT_CLKSEL		(1<<0)

#define CLKSEL_INTERNAL		0
#define CLKSEL_PLL		1

/*------------------------------
	MPU65XX_PWR_MGMT_2
--------------------------------*/
#define BIT_ACCEL_STBY		0x38
#define BIT_GYRO_STBY		0x07
#define BITS_LPA_WAKE_CTRL	0xC0
#define BITS_LPA_WAKE_1HZ	0x00
#define BITS_LPA_WAKE_2HZ	0x40
#define BITS_LPA_WAKE_20HZ	0x80


/*------------------------------
	MPU65XX_ACCEL_INTEL_CTRL
--------------------------------*/
#define BIT_ACCEL_INTEL_EN	0x80
#define BIT_ACCEL_INTEL_MODE	0x40


/*------------------------------
	MPU65XX_USER_CTRL
--------------------------------*/
#define BIT_FIFO_RST		0x04
#define BIT_DMP_RST		0x08
#define BIT_I2C_MST_EN		0x20
#define BIT_FIFO_EN		0x40
#define BIT_DMP_EN		0x80


/*------------------------------
	MPU65XX_FIFO_EN
--------------------------------*/
#define BIT_ACCEL_OUT		0x08
#define BITS_GYRO_OUT		0x70


/*------------------------------
	MPU65XX_INT_PIN_CFG
--------------------------------*/
#define BIT_BYPASS_EN		0x2

/*------------------------------
	MPU65XX_INT_EN/INT_STATUS
--------------------------------*/
#define BIT_FIFO_OVERFLOW	0x80
#define BIT_MOT_INT		0x40
#define BIT_MPU_RDY		0x04
#define BIT_DMP_INT		0x02
#define BIT_RAW_RDY		0x01

#define DMP_START_ADDR		0x400

#define AXIS_NUM	3
#define AXIS_ADC_BYTE	2
#define SENSOR_PACKET	(AXIS_NUM * AXIS_ADC_BYTE)


/*
	self-test parameter
*/
#define DEF_ST_PRECISION		1000
#define DEF_ST_MPU6500_ACCEL_LPF	2
#define DEF_STABLE_TIME_ST		50
#define DEF_SELFTEST_GYRO_FS		(0 << 3)
#define DEF_SELFTEST_ACCEL_FS		(2 << 3)
#define DEF_SELFTEST_6500_ACCEL_FS	(0 << 3)
#define DEF_SW_SELFTEST_GYRO_FS	GFSR_2000DPS
#define DEF_SW_SELFTEST_SENSITIVITY	\
	((2000*DEF_ST_PRECISION)/32768)

#define DEF_SW_SELFTEST_SAMPLE_COUNT	75
#define DEF_SW_SELFTEST_SAMPLE_TIME	75
#define DEF_SW_ACCEL_CAL_SAMPLE_TIME	50
#define DEF_SW_SKIP_COUNT		10

#define DEF_ST_6500_STABLE_TIME		20
#define BYTES_PER_SENSOR		(6)
#define DEF_SELFTEST_SAMPLE_RATE	0
#define DEF_GYRO_WAIT_TIME		50
#define THREE_AXIS			(3)
#define INIT_ST_SAMPLES			200
#define FIFO_COUNT_BYTE			(2)
#define DEF_ST_TRY_TIMES		2
#define REG_6500_XG_ST_DATA		0x0
#define REG_6500_XA_ST_DATA		0xD
#define BITS_SELF_TEST_EN		0xE0

#define DEF_ST_SCALE			(1L << 15)

/*---- MPU6500 Self Test Pass/Fail Criteria ----*/
/* Gyro Offset Max Value (dps) */
#define DEF_GYRO_OFFSET_MAX		20
/* Gyro Self Test Absolute Limits ST_AL (dps) */
#define DEF_GYRO_ST_AL			60
/* Accel Self Test Absolute Limits ST_AL (mg) */
#define DEF_ACCEL_ST_AL_MIN		225
#define DEF_ACCEL_ST_AL_MAX		675
#define DEF_6500_ACCEL_ST_SHIFT_DELTA	500
#define DEF_6500_GYRO_CT_SHIFT_DELTA	500
#define DEF_ST_MPU6500_ACCEL_LPF	2
#define DEF_ST_6500_ACCEL_FS_MG		2000UL
#define DEF_SELFTEST_6500_ACCEL_FS	(0 << 3)

#define DEF_SELFTEST_GYRO_SENS		(32768 / 250)


struct mpu65xx_reg_cfg_t {
	u8 gyro_fsr;
	u8 gyro_dlpf;
	u8 accel_fsr;
	u8 accel_dlpf;
	u8 int_pin_cfg;
	u8 wom_thres;
	u16 prog_start_addr;
};

struct mpu65xx_reg_backup_t {
	u8 pwr_mgmt[2];
	u8 config;
	u8 gyro_config;
	u8 accel_config;
	u8 accel_config2;
	u8 sample_div;
	u8 fifo_en;
	u8 user_ctrl;
	u8 int_enable;
};

struct mpu65xx_ctrl_t {
	u16 i2c_addr;
	struct invsens_i2c_t *i2c_handle;

	struct mpu65xx_reg_cfg_t reg;
	struct mpu65xx_reg_backup_t reg_backup;
	struct invsens_sm_ctrl_t sm_ctrl;

	unsigned int enabled_mask;
	signed char orientation[9];
	int packet_size;

	s16 accel_bias[AXIS_NUM];

	bool is_accel_on_fifo;
	bool is_gyro_on_fifo;

	bool is_mi_enabled;
	bool is_lpa_enabled;
	bool is_pm_suspended;
};



static const u16 mpu65xx_st_tb[256] = {
	2620, 2646, 2672, 2699, 2726, 2753, 2781, 2808,
	2837, 2865, 2894, 2923, 2952, 2981, 3011, 3041,
	3072, 3102, 3133, 3165, 3196, 3228, 3261, 3293,
	3326, 3359, 3393, 3427, 3461, 3496, 3531, 3566,
	3602, 3638, 3674, 3711, 3748, 3786, 3823, 3862,
	3900, 3939, 3979, 4019, 4059, 4099, 4140, 4182,
	4224, 4266, 4308, 4352, 4395, 4439, 4483, 4528,
	4574, 4619, 4665, 4712, 4759, 4807, 4855, 4903,
	4953, 5002, 5052, 5103, 5154, 5205, 5257, 5310,
	5363, 5417, 5471, 5525, 5581, 5636, 5693, 5750,
	5807, 5865, 5924, 5983, 6043, 6104, 6165, 6226,
	6289, 6351, 6415, 6479, 6544, 6609, 6675, 6742,
	6810, 6878, 6946, 7016, 7086, 7157, 7229, 7301,
	7374, 7448, 7522, 7597, 7673, 7750, 7828, 7906,
	7985, 8065, 8145, 8227, 8309, 8392, 8476, 8561,
	8647, 8733, 8820, 8909, 8998, 9088, 9178, 9270,
	9363, 9457, 9551, 9647, 9743, 9841, 9939, 10038,
	10139, 10240, 10343, 10446, 10550, 10656, 10763, 10870,
	10979, 11089, 11200, 11312, 11425, 11539, 11654, 11771,
	11889, 12008, 12128, 12249, 12371, 12495, 12620, 12746,
	12874, 13002, 13132, 13264, 13396, 13530, 13666, 13802,
	13940, 14080, 14221, 14363, 14506, 14652, 14798, 14946,
	15096, 15247, 15399, 15553, 15709, 15866, 16024, 16184,
	16346, 16510, 16675, 16842, 17010, 17180, 17352, 17526,
	17701, 17878, 18057, 18237, 18420, 18604, 18790, 18978,
	19167, 19359, 19553, 19748, 19946, 20145, 20347, 20550,
	20756, 20963, 21173, 21385, 21598, 21814, 22033, 22253,
	22475, 22700, 22927, 23156, 23388, 23622, 23858, 24097,
	24338, 24581, 24827, 25075, 25326, 25579, 25835, 26093,
	26354, 26618, 26884, 27153, 27424, 27699, 27976, 28255,
	28538, 28823, 29112, 29403, 29697, 29994, 30294, 30597,
	30903, 31212, 31524, 31839, 32157, 32479, 32804
};


static int mpu65xx_set_configuration(struct mpu65xx_ctrl_t *dctl)
{
	int res = 0;

	INV_DBG_FUNC_NAME;

	dctl->reg.gyro_fsr = GFSR_2000DPS;
	dctl->reg.gyro_dlpf = DLPF_CFG_41HZ;
	dctl->reg.accel_fsr = AFSR_2G;
	dctl->reg.accel_dlpf = A_DLPF_CFG_41HZ;
	dctl->reg.prog_start_addr = DMP_START_ADDR;
	dctl->reg.wom_thres = 0xc;
	return res;
}

static int mpu65xx_check_silicon_rev(struct mpu65xx_ctrl_t *dctl)
{
	int res = SM_SUCCESS;

	return res;
}

static int mpu65xx_backup_register(struct mpu65xx_ctrl_t *dctl)
{
	int res = SM_SUCCESS;

	INV_DBG_FUNC_NAME;

	i2c_read(dctl, MPU65XX_PWR_MGMT_1, 2,
		dctl->reg_backup.pwr_mgmt, res, exit_i2c_error);

	if (dctl->reg_backup.pwr_mgmt[0] & BIT_SLEEP) {
		i2c_write_single(dctl, MPU65XX_PWR_MGMT_1,
				 dctl->reg_backup.pwr_mgmt[0] & ~BIT_SLEEP,
				 res, exit_i2c_error);
		msleep(30);
	}
	i2c_read(dctl, MPU65XX_CONFIG, 1,
		 &dctl->reg_backup.config, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_GYRO_CONFIG, 1,
		 &dctl->reg_backup.gyro_config, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_ACCEL_CONFIG, 1,
		 &dctl->reg_backup.accel_config, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_ACCEL_CONFIG2, 1,
		 &dctl->reg_backup.accel_config2, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_SMPLRT_DIV, 1,
		 &dctl->reg_backup.sample_div, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_FIFO_EN, 1,
		 &dctl->reg_backup.fifo_en, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_USER_CTRL, 1,
		 &dctl->reg_backup.user_ctrl, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_INT_ENABLE, 1,
		 &dctl->reg_backup.int_enable, res, exit_i2c_error);

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}


static int mpu65xx_restore_register(struct mpu65xx_ctrl_t *dctl)
{
	int res = SM_SUCCESS;

	INV_DBG_FUNC_NAME;

	i2c_write_single(dctl, MPU65XX_CONFIG,
			 dctl->reg_backup.config, res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_GYRO_CONFIG,
			 dctl->reg_backup.gyro_config, res,
			 exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG,
			 dctl->reg_backup.accel_config, res,
			 exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2,
			 dctl->reg_backup.accel_config2, res,
			 exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_SMPLRT_DIV,
			 dctl->reg_backup.sample_div, res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_FIFO_EN, dctl->reg_backup.fifo_en,
			 res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_USER_CTRL,
			 dctl->reg_backup.user_ctrl, res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_INT_ENABLE,
			 dctl->reg_backup.int_enable, res, exit_i2c_error);

	i2c_write_single(dctl, MPU65XX_PWR_MGMT_2,
			 dctl->reg_backup.pwr_mgmt[1], res,
			 exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_PWR_MGMT_1,
			 dctl->reg_backup.pwr_mgmt[0], res,
			 exit_i2c_error);

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}


static unsigned long mpu65xx_sqrt(unsigned long x)
{
	unsigned long op, res, one;

	op = x;
	res = 0;

	one = 1UL << (BITS_PER_LONG - 2);
	while (one > op)
		one >>= 2;

	while (one != 0) {
		if (op >= res + one) {
			op = op - (res + one);
			res = res +  2 * one;
		}
		res /= 2;
		one /= 4;
	}
	return res;
}

static int mpu65xx_sw_selftest(struct mpu65xx_ctrl_t *dctl,
	long scaled_bias[AXIS_NUM], unsigned long scaled_rms[AXIS_NUM],
	int *pkcount)
{
	int res = 0;
	u8 regs[2];
	u8 tmp = 0;
	int fifo_count = 0;
	int packet_size = 0;
	int packet_count = 0;
	int i = 0;
	long gyro_bias[THREE_AXIS] = {0};
	unsigned long gyro_rms[THREE_AXIS] = {0};
	short gyro_data[DEF_SW_SELFTEST_SAMPLE_TIME][THREE_AXIS];

	INV_DBG_FUNC_NAME;

	i2c_read(dctl, MPU65XX_WHOAMI, 1, regs, res, exit_i2c_error);

	INVSENS_LOGD("WHOAMI : 0x%x\n", regs[0]);

	/*-------------------------------------------------*/
	/* Configurate Gyro/Accel                          */
	/*-------------------------------------------------*/
	res = mpu65xx_backup_register(dctl);
	if (res) {
		INVSENS_LOGE("register backup error = %d\n", res);
		goto exit_i2c_error;
	}

	i2c_read(dctl, MPU65XX_PWR_MGMT_1, 2, regs, res, exit_i2c_error);


	tmp = (BIT_ACCEL_STBY);
	i2c_write_single(dctl, MPU65XX_PWR_MGMT_2, tmp, res, exit_i2c_error);
	/*disable interrupt */
	i2c_write_single(dctl, MPU65XX_INT_ENABLE, 0x0, res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_FIFO_EN, 0x0, res, exit_i2c_error);
	/*clear user_ctrl */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, 0x0, res, exit_i2c_error);
	/*reset fifo */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_RST, res,
		exit_i2c_error);
	/*gyro set dlpf = 41hz */
	i2c_write_single(dctl, MPU65XX_CONFIG, DLPF_CFG_41HZ,
		res, exit_i2c_error);
	/*accel set dlpf = 41hz */
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2, A_DLPF_CFG_41HZ,
		res, exit_i2c_error);
	/*set update rate = 1000Hz/(1+SMPLRT_DIV) */
	i2c_write_single(dctl, MPU65XX_SMPLRT_DIV, 0x0, res, exit_i2c_error);
	/*set gyro fsr = 2000dps */
	i2c_write_single(dctl, MPU65XX_GYRO_CONFIG, DEF_SW_SELFTEST_GYRO_FS,
		res, exit_i2c_error);
	/*set accel fsr = 2g */
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG, AFSR_2G, res,
		exit_i2c_error);
	/*enable fifo */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_EN, res,
			 exit_i2c_error);

	msleep(10);

	/*enable fifo */
	tmp = BITS_GYRO_OUT;
	i2c_write_single(dctl, MPU65XX_FIFO_EN, tmp, res, exit_i2c_error);

	/*-------------------------------------------------*/
	/* End of configuration                            */
	/*-------------------------------------------------*/

	msleep(DEF_SW_SELFTEST_SAMPLE_TIME + DEF_SW_SKIP_COUNT);

	/*-------------------------------------------------*/
	/* Polling sensor data from FIFO                   */
	/*-------------------------------------------------*/

	/*turn off fifo */
	i2c_write_single(dctl, MPU65XX_FIFO_EN, 0x0, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_FIFO_COUNTH, 2, regs, res, exit_i2c_error);

	fifo_count = be16_to_cpup((__be16 *) (&regs[0]));

	packet_size = 6;	/* G=>6, A=>6, G+A=>12 */
	packet_count = fifo_count / packet_size;

	packet_count -= DEF_SW_SKIP_COUNT;

	if (packet_count > DEF_SW_SELFTEST_SAMPLE_COUNT)
		packet_count = DEF_SW_SELFTEST_SAMPLE_TIME;

	/*skip first 10 counts to consider stabilization*/
	for (i = 0; i < DEF_SW_SKIP_COUNT; i++) {
		u8 sensor_raw[12];

		i2c_read(dctl, MPU65XX_FIFO_R_W, packet_size,
			 sensor_raw, res, exit_i2c_error);
	}

	for (i = 0; i < packet_count; i++) {
		u8 sensor_raw[12];
		short sensors[6];

		i2c_read(dctl, MPU65XX_FIFO_R_W, packet_size,
			 sensor_raw, res, exit_i2c_error);

		sensors[0] = be16_to_cpup((__be16 *) (&sensor_raw[0]));
		sensors[1] = be16_to_cpup((__be16 *) (&sensor_raw[2]));
		sensors[2] = be16_to_cpup((__be16 *) (&sensor_raw[4]));

		gyro_data[i][0] = sensors[0];
		gyro_data[i][1] = sensors[1];
		gyro_data[i][2] = sensors[2];

		gyro_bias[0] += sensors[0];
		gyro_bias[1] += sensors[1];
		gyro_bias[2] += sensors[2];
	}

	gyro_bias[0] /= packet_count;
	gyro_bias[1] /= packet_count;
	gyro_bias[2] /= packet_count;

	scaled_bias[0] = gyro_bias[0] * DEF_SW_SELFTEST_SENSITIVITY;
	scaled_bias[1] = gyro_bias[1] * DEF_SW_SELFTEST_SENSITIVITY;
	scaled_bias[2] = gyro_bias[2] * DEF_SW_SELFTEST_SENSITIVITY;

	INVSENS_LOGD("gyro bias : %ld %ld %ld\n", gyro_bias[0], gyro_bias[1],
		gyro_bias[2]);

	for (i = 0; i < packet_count; i++) {
		gyro_rms[0] += (gyro_data[i][0] - gyro_bias[0]) *
			(gyro_data[i][0] - gyro_bias[0]);
		gyro_rms[1] += (gyro_data[i][1] - gyro_bias[1]) *
			(gyro_data[i][1] - gyro_bias[1]);
		gyro_rms[2] += (gyro_data[i][2] - gyro_bias[2]) *
			(gyro_data[i][2] - gyro_bias[2]);
	}

	INVSENS_LOGD("gyro rms : %ld %ld %ld\n", gyro_rms[0]/packet_count,
		gyro_rms[1]/packet_count, gyro_rms[2]/packet_count);

	gyro_rms[0] = mpu65xx_sqrt(gyro_rms[0] / packet_count);
	gyro_rms[1] = mpu65xx_sqrt(gyro_rms[1] / packet_count);
	gyro_rms[2] = mpu65xx_sqrt(gyro_rms[2] / packet_count);

	scaled_rms[0] = gyro_rms[0] * DEF_SW_SELFTEST_SENSITIVITY;
	scaled_rms[1] = gyro_rms[1] * DEF_SW_SELFTEST_SENSITIVITY;
	scaled_rms[2] = gyro_rms[2] * DEF_SW_SELFTEST_SENSITIVITY;

	*pkcount = packet_count;

	/*clear int status */
	i2c_read(dctl, MPU65XX_INT_STATUS, 1, regs, res, exit_i2c_error);

	mpu65xx_restore_register(dctl);

	return SM_SUCCESS;

 exit_i2c_error:
	return res;
}



static int mpu65xx_do_test(struct mpu65xx_ctrl_t *dctl, int self_test_flag,
	int gyro_result[THREE_AXIS], int accel_result[THREE_AXIS])
{
	int res, i, j, packet_size;
	u8 data[BYTES_PER_SENSOR * 2], d;
	bool has_accel;
	int fifo_count, packet_count, ind, s;

	packet_size = BYTES_PER_SENSOR*2;
	has_accel = true;

	i2c_write_single(dctl, MPU65XX_INT_ENABLE, 0, res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_FIFO_EN, 0, res, exit_i2c_error);
	/* disable fifo reading */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, 0, res, exit_i2c_error);
	/* clear FIFO */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_RST, res,
			 exit_i2c_error);
	/* setup parameters */
	i2c_write_single(dctl, MPU65XX_CONFIG, DLPF_CFG_98HZ, res,
			 exit_i2c_error);

	/* config accel LPF register for MPU6500 */
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2,
			 DEF_ST_MPU6500_ACCEL_LPF |
			 BIT_FIFO_SIZE_1K, res, exit_i2c_error);


	i2c_write_single(dctl, MPU65XX_SMPLRT_DIV,
			 DEF_SELFTEST_SAMPLE_RATE, res, exit_i2c_error);
	/* wait for the sampling rate change to stabilize */
	mdelay(DEF_STABLE_TIME_ST);
	i2c_write_single(dctl, MPU65XX_GYRO_CONFIG,
			 (self_test_flag | DEF_SELFTEST_GYRO_FS), res,
			 exit_i2c_error);

	if (has_accel) {
		d = DEF_SELFTEST_6500_ACCEL_FS;

		d |= self_test_flag;
		i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG, d, res,
				 exit_i2c_error);
	}
	/* wait for the output to get stable */
	if (self_test_flag)
		msleep(DEF_ST_6500_STABLE_TIME);

	/* enable FIFO reading */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_EN, res,
			 exit_i2c_error);
	/* enable sensor output to FIFO */
	if (has_accel)
		d = BITS_GYRO_OUT | BIT_ACCEL_OUT;
	else
		d = BITS_GYRO_OUT;
	for (i = 0; i < THREE_AXIS; i++) {
		gyro_result[i] = 0;
		accel_result[i] = 0;
	}
	s = 0;
	while (s < INIT_ST_SAMPLES) {
		i2c_write_single(dctl, MPU65XX_FIFO_EN, d, res,
				 exit_i2c_error);
		mdelay(DEF_GYRO_WAIT_TIME);
		i2c_write_single(dctl, MPU65XX_FIFO_EN, 0, res,
				 exit_i2c_error);

		i2c_read(dctl, MPU65XX_FIFO_COUNTH,
			 FIFO_COUNT_BYTE, data, res, exit_i2c_error);
		fifo_count = be16_to_cpup((__be16 *) (&data[0]));

		packet_count = fifo_count / packet_size;
		i = 0;
		while (i < packet_count) {
			short vals[3];
			i2c_read(dctl, MPU65XX_FIFO_R_W,
				 packet_size, data, res, exit_i2c_error);
			ind = 0;
			if (has_accel) {
				for (j = 0; j < THREE_AXIS; j++) {
					vals[j] = (short)
						be16_to_cpup((__be16 *)(&data[ind + 2*j]));
					accel_result[j] += vals[j];
				}
				ind += BYTES_PER_SENSOR;
			}

			for (j = 0; j < THREE_AXIS; j++) {
				vals[j] = (short)
					be16_to_cpup((__be16 *)(&data[ind + 2*j]));
				gyro_result[j] += vals[j];
			}

			s++;
			i++;
		}
	}

	if (has_accel) {
		for (j = 0; j < THREE_AXIS; j++) {
			accel_result[j] = accel_result[j] / s;
			accel_result[j] *= DEF_ST_PRECISION;
		}
	}
	for (j = 0; j < THREE_AXIS; j++) {
		gyro_result[j] = gyro_result[j] / s;
		gyro_result[j] *= DEF_ST_PRECISION;
	}

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}

static int mpu65xx_gyro_self_test(struct mpu65xx_ctrl_t *dctl,
	int *reg_avg, int *st_avg, int *st_ratio)
{
	u8 regs[3];
	int ret_val, result;
	int otp_value_zero = 0;
	int st_shift_prod[3], st_shift_cust[3], i;

	ret_val = 0;
	result = i2c_read_reg(dctl, REG_6500_XG_ST_DATA, 3, regs);
	if (result)
		return result;
	INVSENS_LOGD("self_test gyro shift_code - %02x %02x %02x\n",
		 regs[0], regs[1], regs[2]);

	for (i = 0; i < 3; i++) {
		if (regs[i] != 0) {
			st_shift_prod[i] = mpu65xx_st_tb[regs[i] - 1];
		} else {
			st_shift_prod[i] = 0;
			otp_value_zero = 1;
		}
	}
	INVSENS_LOGD("self_test gyro st_shift_prod - %+d %+d %+d\n",
		 st_shift_prod[0], st_shift_prod[1],
		 st_shift_prod[2]);

	for (i = 0; i < 3; i++) {
		st_shift_cust[i] = st_avg[i] - reg_avg[i];
		if (!otp_value_zero) {
			/* Self Test Pass/Fail Criteria A */
			if (st_shift_cust[i] < DEF_6500_GYRO_CT_SHIFT_DELTA
						* st_shift_prod[i])
					ret_val = 1;
		} else {
			/* Self Test Pass/Fail Criteria B */
			if (st_shift_cust[i] < DEF_GYRO_ST_AL *
						DEF_SELFTEST_GYRO_SENS *
						DEF_ST_PRECISION)
				ret_val = 1;
		}

		if (st_ratio)
			st_ratio[i] = abs(st_shift_cust[i] / st_shift_prod[i]
				- DEF_ST_PRECISION);
	}
	INVSENS_LOGD("self_test gyro st_shift_cust - %+d %+d %+d\n",
		 st_shift_cust[0], st_shift_cust[1],
		 st_shift_cust[2]);

	if (ret_val == 0) {
		/* Self Test Pass/Fail Criteria C */
		for (i = 0; i < 3; i++)
			if (abs(reg_avg[i]) > DEF_GYRO_OFFSET_MAX *
						DEF_SELFTEST_GYRO_SENS *
						DEF_ST_PRECISION)
				ret_val = 1;
	}

	return ret_val;
}


static int mpu65xx_accel_self_test(struct mpu65xx_ctrl_t *dctl,
	int *reg_avg, int *st_avg, int st_ratio[3])
{
	int ret_val, result;
	int st_shift_prod[3], st_shift_cust[3], st_shift_ratio[3], i;
	u8 regs[3];
	int otp_value_zero = 0;

#define ACCEL_ST_AL_MIN ((DEF_ACCEL_ST_AL_MIN * DEF_ST_SCALE \
				 / DEF_ST_6500_ACCEL_FS_MG) * DEF_ST_PRECISION)
#define ACCEL_ST_AL_MAX ((DEF_ACCEL_ST_AL_MAX * DEF_ST_SCALE \
				 / DEF_ST_6500_ACCEL_FS_MG) * DEF_ST_PRECISION)

	ret_val = 0;
	result = i2c_read_reg(dctl, REG_6500_XA_ST_DATA, 3, regs);
	if (result)
		return result;
	INVSENS_LOGD("self_test accel shift_code - %02x %02x %02x\n",
		 regs[0], regs[1], regs[2]);

	for (i = 0; i < 3; i++) {
		if (regs[i] != 0) {
			st_shift_prod[i] = mpu65xx_st_tb[regs[i] - 1];
		} else {
			st_shift_prod[i] = 0;
			otp_value_zero = 1;
		}
	}
	INVSENS_LOGD("self_test accel st_shift_prod - %+d %+d %+d\n",
		 st_shift_prod[0], st_shift_prod[1],
		 st_shift_prod[2]);

	if (!otp_value_zero) {
		/* Self Test Pass/Fail Criteria A */
		for (i = 0; i < 3; i++) {
			st_shift_cust[i] = st_avg[i] - reg_avg[i];
			st_shift_ratio[i] = abs(st_shift_cust[i] /
					st_shift_prod[i] - DEF_ST_PRECISION);
			if (st_shift_ratio[i] > DEF_6500_ACCEL_ST_SHIFT_DELTA)
				ret_val = 1;

			if (st_ratio)
				st_ratio[i] = st_shift_ratio[i];
		}
	} else {
		/* Self Test Pass/Fail Criteria B */
		for (i = 0; i < 3; i++) {
			st_shift_cust[i] = abs(st_avg[i] - reg_avg[i]);
			if (st_shift_cust[i] < ACCEL_ST_AL_MIN ||
					st_shift_cust[i] > ACCEL_ST_AL_MAX)
				ret_val = 1;

			if (st_ratio)
				st_ratio[i] = 0;
		}
	}
	INVSENS_LOGD("self_test accel st_shift_cust - %+d %+d %+d\n",
		 st_shift_cust[0], st_shift_cust[1],
		 st_shift_cust[2]);

	return ret_val;
}


static int mpu65xx_hw_selftest(struct mpu65xx_ctrl_t *dctl,
	int *accel_st_ratio, int *gyro_st_ratio)
{
	int res = 0;
	u8 regs[2];
	int gyro_bias_st[THREE_AXIS], gyro_bias_regular[THREE_AXIS];
	int accel_bias_st[THREE_AXIS], accel_bias_regular[THREE_AXIS];
	int test_times;
	char compass_result, accel_result, gyro_result;


	INV_DBG_FUNC_NAME;

	i2c_read(dctl, MPU65XX_WHOAMI, 1, regs, res, exit_i2c_error);

	INVSENS_LOGD("WHOAMI : 0x%x\n", regs[0]);

	/*-------------------------------------------------*/
	/* Configurate Gyro/Accel                          */
	/*-------------------------------------------------*/
	res = mpu65xx_backup_register(dctl);
	if (res) {
		INVSENS_LOGE("register backup error = %d\n", res);
		goto exit_i2c_error;
	}

	i2c_read(dctl, MPU65XX_PWR_MGMT_2, 1, regs, res, exit_i2c_error);

	regs[0] &= ~((BIT_ACCEL_STBY) | (BIT_GYRO_STBY));

	i2c_write_single(dctl, MPU65XX_PWR_MGMT_2, regs[0],
		res, exit_i2c_error);

	compass_result = 0;
	accel_result = 0;
	gyro_result = 0;
	test_times = DEF_ST_TRY_TIMES;
	while (test_times > 0) {
		res = mpu65xx_do_test(dctl, 0, gyro_bias_regular,
			accel_bias_regular);
		if (res == -EAGAIN)
			test_times--;
		else
			test_times = 0;
	}
	if (res)
		goto exit_st_error;

	INVSENS_LOGD("self_test accel bias_regular - %+d %+d %+d\n",
		 accel_bias_regular[0],
		 accel_bias_regular[1], accel_bias_regular[2]);
	INVSENS_LOGD("self_test gyro bias_regular - %+d %+d %+d\n",
		 gyro_bias_regular[0], gyro_bias_regular[1],
		 gyro_bias_regular[2]);
	test_times = DEF_ST_TRY_TIMES;
	while (test_times > 0) {
		res = mpu65xx_do_test(dctl, BITS_SELF_TEST_EN, gyro_bias_st,
					accel_bias_st);
		if (res == -EAGAIN)
			test_times--;
		else
			break;
	}
	INVSENS_LOGD("self_test accel bias_st - %+d %+d %+d\n",
		 accel_bias_st[0], accel_bias_st[1],
		 accel_bias_st[2]);
	INVSENS_LOGD("self_test gyro bias_st - %+d %+d %+d\n",
		 gyro_bias_st[0], gyro_bias_st[1],
		 gyro_bias_st[2]);

	accel_result = !mpu65xx_accel_self_test(dctl,
		accel_bias_regular, accel_bias_st, accel_st_ratio);
	gyro_result = !mpu65xx_gyro_self_test(dctl,
		gyro_bias_regular, gyro_bias_st, gyro_st_ratio);

	/*clear int status */
	i2c_read(dctl, MPU65XX_INT_STATUS, 1, regs, res, exit_i2c_error);

	mpu65xx_restore_register(dctl);

	return (accel_result << 1) | gyro_result;

exit_st_error:
exit_i2c_error:
	return res;
}

static int mpu65xx_setup_hw(struct mpu65xx_ctrl_t *dctl)
{
	int res = 0;
	u8 regs[2];

	INV_DBG_FUNC_NAME;

	mpu65xx_set_configuration(dctl);

	/*reset hw */
	i2c_write_single(dctl, MPU65XX_PWR_MGMT_1, BIT_H_RESET, res,
			 exit_i2c_error);

	mdelay(30);

	/*wake hw */
	i2c_write_single(dctl, MPU65XX_PWR_MGMT_1, 0x0, res, exit_i2c_error);

	/*check silicon revision */
	res = mpu65xx_check_silicon_rev(dctl);

	/*set gyro fsr */
	regs[0] = dctl->reg.gyro_fsr;
	i2c_write_single(dctl,
		MPU65XX_GYRO_CONFIG, regs[0], res, exit_i2c_error);

	/*set gyro dlpf */
	regs[0] = dctl->reg.gyro_dlpf;
	i2c_write_single(dctl, MPU65XX_CONFIG, regs[0], res, exit_i2c_error);

	/*set accel fsr */
	regs[0] = dctl->reg.accel_fsr;
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG, regs[0], res,
			 exit_i2c_error);

	/*set accel dlpf */
	regs[0] = dctl->reg.accel_dlpf;
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2, regs[0], res,
			 exit_i2c_error);

	/*int cfg : set bypass as defaut */
	regs[0] = dctl->reg.int_pin_cfg;
	i2c_write_single(dctl,
		MPU65XX_INT_PIN_CFG, regs[0], res, exit_i2c_error);

	regs[0] = dctl->reg.prog_start_addr >> 8;
	regs[1] = dctl->reg.prog_start_addr & 0xFF;
	i2c_write_single(dctl, MPU65XX_PRGM_STRT_ADDRH, regs[0], res,
			 exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_PRGM_STRT_ADDRH + 1, regs[1], res,
			 exit_i2c_error);

	/*set sensors stand-by */
	regs[0] = (BIT_ACCEL_STBY | BIT_GYRO_STBY);
	i2c_write_single(dctl,
		MPU65XX_PWR_MGMT_2, regs[0], res, exit_i2c_error);


exit_i2c_error:
	return res;
}




static int mpu65xx_set_delay(struct mpu65xx_ctrl_t *dctl, u32 func_mask,
			     long delay, bool reset)
{
	int res = 0;
	u8 reg = 0;

	long current_delay = LONG_MAX;

	INV_DBG_FUNC_NAME;

	if (dctl->enabled_mask) {
		if (dctl->sm_ctrl.is_dmp_on) {
			reg = INV_DMP_RATE_DIV;
		} else {
			if (dctl->enabled_mask & (1 << INV_FUNC_ACCEL))
				current_delay = INV_MIN(current_delay,
					dctl->sm_ctrl.delays[INV_FUNC_ACCEL]);
			if (dctl->enabled_mask & (1 << INV_FUNC_GYRO))
				current_delay = INV_MIN(current_delay,
					dctl->sm_ctrl.delays[INV_FUNC_GYRO]);
			if (dctl->enabled_mask & (1 << INV_FUNC_FT_SENSOR_ON))
				current_delay = INV_MIN(current_delay,
					dctl->sm_ctrl.delays[INV_FUNC_FT_SENSOR_ON]);
			reg = (u8) (current_delay / 1000000L);
		}
		i2c_write_single(dctl, MPU65XX_SMPLRT_DIV,
			reg, res, exit_i2c_error);

		INVSENS_LOGD("mpu65xx delay = %d\n", reg);

		reg = 0;
		if (dctl->sm_ctrl.is_dmp_on)
			reg = BIT_DMP_INT;
		else if(!dctl->is_lpa_enabled)
			reg = BIT_RAW_RDY;

		if (dctl->is_mi_enabled)
			reg |= BIT_MOT_INT;

		reg |= BIT_FIFO_OVERFLOW;

		i2c_write_single(dctl, MPU65XX_INT_ENABLE,
			reg, res, exit_i2c_error);

	} else {
		i2c_write_single(dctl,
			MPU65XX_INT_ENABLE, reg, res, exit_i2c_error);
	}

	return 0;

exit_i2c_error:
	return res;
}


static int mpu65xx_set_motion_int(struct mpu65xx_ctrl_t *dctl)
{
	int res = SM_SUCCESS;
	u8 accel_config2, accel_intel_ctrl, wom, odr;

	INV_DBG_FUNC_NAME;
	i2c_read(dctl, MPU65XX_ACCEL_CONFIG2, 1, &accel_config2,
		res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_ACCEL_INTEL_CTRL, 1, &accel_intel_ctrl,
		res, exit_i2c_error);

	/*set lpa mode*/
	if (dctl->is_lpa_enabled) {
		accel_config2 |= BIT_ACCEL_FCHOICE_B;
		i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2, accel_config2,
				 res, exit_i2c_error);

		wom = dctl->reg.wom_thres;
		i2c_write_single(dctl,
			MPU65XX_WOM_THRESH, wom, res, exit_i2c_error);

		odr = LPA_CLK_3P91HZ;
		i2c_write_single(dctl,
			MPU65XX_LP_ACCEL_ODR, odr, res, exit_i2c_error);

	} else {
		accel_config2 &= ~BIT_ACCEL_FCHOICE_B;
		i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2,
			accel_config2, res, exit_i2c_error);
	}

	if(dctl->is_mi_enabled) {
		wom = dctl->reg.wom_thres;
		i2c_write_single(dctl, MPU65XX_WOM_THRESH, wom, res, exit_i2c_error);
	}

	/*set motion interrupt*/
	if (dctl->is_mi_enabled) {
		accel_intel_ctrl |= BIT_ACCEL_INTEL_EN;
		accel_intel_ctrl |= BIT_ACCEL_INTEL_MODE;
		i2c_write_single(dctl, MPU65XX_ACCEL_INTEL_CTRL,
			accel_intel_ctrl, res, exit_i2c_error);
	} else {
		accel_intel_ctrl &= ~BIT_ACCEL_INTEL_EN;
		accel_intel_ctrl &= ~BIT_ACCEL_INTEL_MODE;
		i2c_write_single(dctl, MPU65XX_ACCEL_INTEL_CTRL,
			accel_intel_ctrl, res, exit_i2c_error);
	}
	return SM_SUCCESS;

exit_i2c_error:
	return res;
}

static int mpu65xx_enable(struct mpu65xx_ctrl_t *dctl,
				 u32 func_mask, bool enable)
{
	int res = 0;
	u8 pwr_mgmt[2];
	u8 fifo_en, user_ctrl;
	unsigned int mask;
	bool is_accel_active = false;
	bool is_gyro_active = false;
	bool is_slave_active = false;

	INV_DBG_FUNC_NAME;

	if (!dctl)
		return -EINVAL;

	mask = func_mask;

	pr_info("[INV]%s: mask = %d\n", __func__, mask);

	if (mask & (1 << INV_FUNC_ACCEL))
		is_accel_active = true;
	if (mask & (1 << INV_FUNC_GYRO))
		is_gyro_active = true;
	if (mask & (1 << INV_FUNC_COMPASS))
		is_slave_active = true;
	if (mask & (1 << INV_FUNC_FT_SENSOR_ON)) {
		is_accel_active = true;
		is_gyro_active = true;
	}
	if (mask & (1 << INV_FUNC_MOTION_INTERRUPT))
		is_accel_active = true;

	/*-------------------------------------------------*/
	/* check motion interrupt                          */
	/*-------------------------------------------------*/
	if(mask & (1 << INV_FUNC_MOTION_INTERRUPT)) {
		dctl->is_mi_enabled = true;
		if (mask & ~(1 << INV_FUNC_MOTION_INTERRUPT))
			dctl->is_lpa_enabled = false;
		else
			dctl->is_lpa_enabled = true;
	} else {
		dctl->is_mi_enabled = false;
		dctl->is_lpa_enabled = false;
	}

	/*-------------------------------------------------*/
	/* configurate gyro/accel                          */
	/*-------------------------------------------------*/
	i2c_read(dctl, MPU65XX_PWR_MGMT_1, 2, pwr_mgmt, res, exit_i2c_error);

	/*remove cycle bit as default*/
	pwr_mgmt[0] &= ~(BIT_CYCLE | BIT_CLKSEL);

	if (!mask) {
		/*disable interrupt */
		i2c_write_single(dctl, MPU65XX_INT_ENABLE,
				 0x0, res, exit_i2c_error);

		/*all functions are disabled, chip can go to sleep*/
		i2c_write_single(dctl, MPU65XX_PWR_MGMT_1,
				 pwr_mgmt[0] | BIT_SLEEP, res, exit_i2c_error);
		INVSENS_LOGI("sleep device : mpu65xx\n");
	} else {
		/*there is something to enable, wake up chip!*/
		if (is_gyro_active)
			pwr_mgmt[0] |= CLKSEL_PLL;

		if (pwr_mgmt[0] & BIT_SLEEP) {
			pwr_mgmt[0] &= ~BIT_SLEEP;
			i2c_write_single(dctl, MPU65XX_PWR_MGMT_1,
				pwr_mgmt[0], res, exit_i2c_error);
			msleep(30);
		} else {
			i2c_write_single(dctl, MPU65XX_PWR_MGMT_1,
					 pwr_mgmt[0], res, exit_i2c_error);
		}

		/*config pwr_mgmt_2*/
		pwr_mgmt[1] |= ((BIT_ACCEL_STBY) | (BIT_GYRO_STBY));

		if (is_accel_active)
			pwr_mgmt[1] &= ~(BIT_ACCEL_STBY);

		if (is_gyro_active)
			pwr_mgmt[1] &= ~(BIT_GYRO_STBY);

		if(dctl->is_lpa_enabled)
			pwr_mgmt[1] |= BITS_LPA_WAKE_20HZ;

		i2c_write_single(dctl, MPU65XX_PWR_MGMT_2, pwr_mgmt[1],
			res, exit_i2c_error);

		/*disable interrupt */
		i2c_write_single(dctl, MPU65XX_INT_ENABLE, 0x0,
			res, exit_i2c_error);

		/*turn off fifo */
		i2c_write_single(dctl, MPU65XX_FIFO_EN, 0x0,
			res, exit_i2c_error);

		/*clear user_ctrl */
		i2c_write_single(dctl, MPU65XX_SMPLRT_DIV, 0xff,
			res, exit_i2c_error);

		/*reset fifo */
		user_ctrl = BIT_FIFO_RST;
		if (dctl->sm_ctrl.is_dmp_on)
			user_ctrl |= BIT_DMP_RST;
		i2c_write_single(dctl, MPU65XX_USER_CTRL,
			user_ctrl, res, exit_i2c_error);


		/*set user_ctrl */
		user_ctrl = 0;
		if(!dctl->is_lpa_enabled) {
			if (is_accel_active | is_gyro_active)
				user_ctrl = BIT_FIFO_EN;

			if (is_slave_active)
				user_ctrl |= BIT_I2C_MST_EN | BIT_FIFO_EN;

			if (user_ctrl && dctl->sm_ctrl.is_dmp_on)
				user_ctrl |= BIT_DMP_EN;
		}

		if (user_ctrl)
			i2c_write_single(dctl, MPU65XX_USER_CTRL,
				user_ctrl, res, exit_i2c_error);

		/*enable fifo */
		fifo_en = 0;
		dctl->is_accel_on_fifo = false;
		dctl->is_gyro_on_fifo = false;

		if (!dctl->sm_ctrl.is_dmp_on) {
			dctl->packet_size = 0;

			if (is_accel_active) {
				fifo_en |= BIT_ACCEL_OUT;
				dctl->packet_size += SENSOR_PACKET;
				dctl->is_accel_on_fifo = true;
			}
			if (is_gyro_active) {
				fifo_en |= BITS_GYRO_OUT;
				dctl->packet_size += SENSOR_PACKET;
				dctl->is_gyro_on_fifo = true;
			}
		}

		if (fifo_en)
			i2c_write_single(dctl, MPU65XX_FIFO_EN,
				fifo_en, res, exit_i2c_error);

		/*set motion interrupt*/
		mpu65xx_set_motion_int(dctl);

		if (dctl->is_lpa_enabled)
			i2c_write_single(dctl, MPU65XX_PWR_MGMT_1,
				pwr_mgmt[0] | BIT_CYCLE, res, exit_i2c_error);

	}

	if (!res) {
		dctl->enabled_mask = mask;

		mpu65xx_set_delay(dctl, 0, 0, true);
	}

exit_i2c_error:

	return res;
}





static int mpu65xx_apply_orientation(struct mpu65xx_ctrl_t *dctl,
				     const u8 src[], u8 dest[])
{
	short tmp[AXIS_NUM];
	int oriented[AXIS_NUM];

	int i = 0;

	tmp[0] = ((short) src[0] << 8) + src[1];
	tmp[1] = ((short) src[2] << 8) + src[3];
	tmp[2] = ((short) src[4] << 8) + src[5];

	for (i = 0; i < AXIS_NUM; i++) {
		oriented[i] = tmp[0] * dctl->orientation[i * 3 + 0]
		    + tmp[1] * dctl->orientation[i * 3 + 1]
		    + tmp[2] * dctl->orientation[i * 3 + 2];

		/*handle overflow */
		if (oriented[i] > 32767)
			oriented[i] = 32767;
	}

	for (i = 0; i < AXIS_NUM; i++)
		tmp[i] = (short) oriented[i];

	memcpy((void *) dest, (void *) tmp, sizeof(tmp));

	return 0;
}



static int mpu65xx_check_dmp_int(struct mpu65xx_ctrl_t *dctl,
				 u8 dmp_int,
				 struct invsens_data_list_t *buffer)
{
	if (dmp_int & 0x04) {
		/*significat motion detect */
		struct invsens_data_t *d = &buffer->items[buffer->count++];
		d->hdr = INV_DATA_HDR_SMD;
		d->length = 0;
		INVSENS_LOGD("SMD_HDR\n");
	}

	return SM_SUCCESS;
}

static int mpu65xx_reset_fifo(struct mpu65xx_ctrl_t *dctl)
{
	int res;
	u8 uctl;

	i2c_read_reg(dctl, MPU65XX_USER_CTRL, 1, &uctl);
	/*clear user_ctrl */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, 0x0, res, exit_i2c_error);
	/*reset fifo */
	i2c_write_single(dctl, MPU65XX_USER_CTRL,
		BIT_FIFO_RST | BIT_DMP_RST, res, exit_i2c_error);

	mdelay(1);

	/*reset fifo */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, uctl, res,
		exit_i2c_error);

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}

static int mpu65xx_read(struct mpu65xx_ctrl_t *dctl,
			struct invsens_data_list_t *buffer)
{
	int res = 0, i = 0;
	u8 regs[2];
	u8 int_stat[2];
	u16 count = 0;
	int max_buffer = INVSENS_DATA_ITEM_MAX - buffer->count;

#define BUFFER_LEN  256
#define DMP_INT (0)
#define MPU_INT (1)

	u8 buf[BUFFER_LEN];
	struct invsens_data_t *p = NULL;

	INV_DBG_FUNC_NAME;

	if (dctl->is_pm_suspended)
		return SM_SUCCESS;

	/*not enough buffer*/
	if(max_buffer<=0)
		return SM_SUCCESS;

	res = i2c_read_reg(dctl, MPU65XX_DMP_INT_STATUS, 2, int_stat);
	if (res)
		goto exit_i2c_error;

	res = i2c_read_reg(dctl, MPU65XX_FIFO_COUNTH, 2, regs);
	if (res)
		goto exit_i2c_error;

	count = ((u16) regs[0] << 8) + regs[1];

	if(int_stat[1] & 0x10)
		mpu65xx_reset_fifo(dctl);

	if (dctl->sm_ctrl.is_dmp_on) {
		/*
		* if dmp is on, and count is not zero,
		* fifo memory copied on copy_buffer,
		* transfer to fw driver of dmp.
		* because, mpu can't know the fifo format.
		*/
		count -= count % INVSENS_DMP_DATUM;

		/*recalculate byte count to leave remain data into fifo*/
		count = INV_MIN(count, max_buffer*INVSENS_DMP_DATUM);

		if (count > 0 && buffer->copy_buffer) {
			i2c_read(dctl, MPU65XX_FIFO_R_W, count,
				buffer->copy_buffer, res, exit_i2c_error);
			buffer->is_fifo_data_copied = true;
			buffer->fifo_data_length = (u16)count;
		}

		/*
		* handle dmp interrupt
		*/
		if (int_stat[DMP_INT]) {
			u8 dmp_int = int_stat[DMP_INT];
			INVSENS_LOGD("dmp int = 0x%x\n", dmp_int);

			mpu65xx_check_dmp_int(dctl, dmp_int, buffer);
		}
	} else {
		if (count > 0) {
			int packet_num = count / dctl->packet_size;

			/*reset packet number to consider the buffer size of data list*/
			packet_num = INV_MIN(packet_num, max_buffer);

			do {
				i2c_read(dctl, MPU65XX_FIFO_R_W,
					dctl->packet_size, buf, res,
					exit_i2c_error);
				i = 0;

				if (dctl->is_accel_on_fifo) {
					p = &buffer->items[buffer->count++];
					p->hdr = INV_DATA_HDR_ACCEL;
					p->length = SENSOR_PACKET;
					mpu65xx_apply_orientation(dctl,
						&buf[i], &p->data[0]);
					i += SENSOR_PACKET;
				}

				if (dctl->is_gyro_on_fifo) {
					p = &buffer->items[buffer->count++];
					p->hdr = INV_DATA_HDR_GYRO;
					p->length = SENSOR_PACKET;
					mpu65xx_apply_orientation(dctl,
						&buf[i], &p->data[0]);
					i += SENSOR_PACKET;
				}

				packet_num--;
			} while (packet_num > 0);
		}
	}

	if (int_stat[MPU_INT] & BIT_MOT_INT) {
		if(dctl->is_mi_enabled) {
			buffer->event_motion_interrupt_notified = true;
			INVSENS_LOGD("motion interrupt !!!!!\n");
		}
	}


exit_i2c_error:
	return res;
}


static int mpu65xx_read_accel_raw(struct mpu65xx_ctrl_t *dctl,
	short raw[AXIS_NUM])
{
	int res = SM_SUCCESS;
	u8 d[SENSOR_PACKET];

	i2c_read(dctl, MPU65XX_ACCEL_XOUT_H, SENSOR_PACKET, d, res,
		exit_i2c_error);

	mpu65xx_apply_orientation(dctl, d, (u8 *)raw);

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}


static int mpu65xx_read_gyro_raw(struct mpu65xx_ctrl_t *dctl,
	short raw[AXIS_NUM])
{
	int res = SM_SUCCESS;
	u8 d[SENSOR_PACKET];

	i2c_read(dctl, MPU65XX_GYRO_XOUT_H, SENSOR_PACKET, d, res,
		exit_i2c_error);

	mpu65xx_apply_orientation(dctl, d, (u8 *)raw);

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}



static int mpu65xx_get_temperature(struct mpu65xx_ctrl_t *dctl, int *temp)
{
	int res = SM_SUCCESS;
	u8 regs[2];
	int temperature;

	INV_DBG_FUNC_NAME;


	i2c_read(dctl, MPU65XX_TEMP_OUT_H, 2,
			regs, res, exit_i2c_error);

	temperature = (short) (((regs[0]) << 8) | regs[1]);
	temperature = ((temperature + 521) / 340) + 35;

	*temp = temperature;

	return SM_SUCCESS;

exit_i2c_error:

	return res;
}

static int mpu65xx_do_accel_calib(struct mpu65xx_ctrl_t *dctl, short *bias)
{
	int res;
	u8 regs[2], tmp;
	int i, packet_size = 0, packet_count = 0, fifo_count;
	short criteria[3] = {0, 0, 16384};
	long accel_bias[3];

	INV_DBG_FUNC_NAME;

	res = mpu65xx_backup_register(dctl);
	if (res) {
		INVSENS_LOGE("register backup error = %d\n", res);
		goto exit_i2c_error;
	}
	i2c_read(dctl, MPU65XX_PWR_MGMT_1, 2, regs, res, exit_i2c_error);

	tmp = (BIT_GYRO_STBY);
	i2c_write_single(dctl, MPU65XX_PWR_MGMT_2, tmp, res, exit_i2c_error);
	/*disable interrupt */
	i2c_write_single(dctl, MPU65XX_INT_ENABLE, 0x0, res, exit_i2c_error);
	i2c_write_single(dctl, MPU65XX_FIFO_EN, 0x0, res, exit_i2c_error);
	/*clear user_ctrl */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, 0x0, res, exit_i2c_error);
	/*reset fifo */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_RST, res,
		exit_i2c_error);
	/*accel set dlpf = 41hz */
	i2c_write_single(dctl, MPU65XX_ACCEL_CONFIG2, A_DLPF_CFG_41HZ,
			 res, exit_i2c_error);
	/*set update rate = 1000Hz/(1+SMPLRT_DIV) */
	i2c_write_single(dctl, MPU65XX_SMPLRT_DIV, 0x0, res, exit_i2c_error);
	/*enable fifo */
	i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_EN, res,
			 exit_i2c_error);

	msleep(10);

	/*enable fifo */
	tmp = BIT_ACCEL_OUT;
	i2c_write_single(dctl, MPU65XX_FIFO_EN, tmp, res, exit_i2c_error);

	/*-------------------------------------------------*/
	/* End of configuration                                              */
	/*-------------------------------------------------*/

	msleep(DEF_SW_ACCEL_CAL_SAMPLE_TIME);

	/*-------------------------------------------------*/
	/* Polling sensor data from FIFO                                      */
	/*-------------------------------------------------*/

	/*turn off fifo */
	i2c_write_single(dctl, MPU65XX_FIFO_EN, 0x0, res, exit_i2c_error);
	i2c_read(dctl, MPU65XX_FIFO_COUNTH, 2, regs, res, exit_i2c_error);

	fifo_count = be16_to_cpup((__be16 *) (&regs[0]));

	packet_size = 6;	/*A=>6*/
	packet_count = fifo_count / packet_size;

	memset(accel_bias, 0x0, sizeof(accel_bias));

	for (i = 0; i < packet_count; i++) {
		u8 sensor_raw[12];
		short sensors[6];

		i2c_read(dctl, MPU65XX_FIFO_R_W, packet_size,
			 sensor_raw, res, exit_i2c_error);

		mpu65xx_apply_orientation(dctl, sensor_raw, (u8 *)sensors);
		INVSENS_LOGI("accel data : %d, %d, %d\n",
			sensors[0], sensors[1], sensors[2]);

		accel_bias[0] += (sensors[0] - criteria[0]);
		accel_bias[1] += (sensors[1] - criteria[1]);
		accel_bias[2] += (sensors[2] - criteria[2]);
	}

	if (bias) {
		bias[0] = (short)(accel_bias[0]/packet_count);
		bias[1] = (short)(accel_bias[1]/packet_count);
		bias[2] = (short)(accel_bias[2]/packet_count);
		INVSENS_LOGI("accel bias : %d, %d, %d\n",
			bias[0], bias[1], bias[2]);
	}

	/*clear int status */
	i2c_read(dctl, MPU65XX_INT_STATUS, 1, regs, res, exit_i2c_error);
	mpu65xx_restore_register(dctl);

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}

static int mpu65xx_resume(struct mpu65xx_ctrl_t *dctl)
{
	int res;

	if (!dctl->sm_ctrl.is_dmp_on) {
		if ((dctl->enabled_mask & (1 << INV_FUNC_ACCEL))
			|| (dctl->enabled_mask & (1 << INV_FUNC_GYRO))) {
			u8 reg = 0;
			i2c_write_single(dctl, MPU65XX_USER_CTRL,
				BIT_FIFO_EN | BIT_FIFO_RST,
				res, exit_i2c_error);
			if (dctl->sm_ctrl.is_dmp_on)
				reg = BIT_DMP_INT;
			else
				reg = BIT_RAW_RDY;

			if (dctl->is_mi_enabled)
				reg |= BIT_MOT_INT;
			i2c_write_single(dctl, MPU65XX_INT_ENABLE, reg,
				res, exit_i2c_error);
		}
	}

	dctl->is_pm_suspended = false;

	return SM_SUCCESS;

exit_i2c_error:

	return res;

}

static int mpu65xx_suspend(struct mpu65xx_ctrl_t *dctl)
{
	int res;
	if (!dctl->sm_ctrl.is_dmp_on) {
		u8 reg = 0;
		if (dctl->is_mi_enabled)
			reg |= BIT_MOT_INT;

		i2c_write_single(dctl, MPU65XX_INT_ENABLE, reg,
			res, exit_i2c_error);
		if (!reg)
			dctl->is_pm_suspended = true;

		i2c_write_single(dctl, MPU65XX_USER_CTRL, BIT_FIFO_RST,
			res, exit_i2c_error);
	}

	return SM_SUCCESS;

exit_i2c_error:
	return res;
}


static int mpu65xx_if_init(struct invsens_driver_t *drv)
{
	int res = 0;
	struct mpu65xx_ctrl_t *dctl;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	res = mpu65xx_setup_hw(dctl);
	if (res != SM_SUCCESS)
		goto exit_hw_setup;

	dctl->is_accel_on_fifo = false;
	dctl->is_gyro_on_fifo = false;

	dctl->is_lpa_enabled = false;
	dctl->is_mi_enabled = false;
	dctl->is_pm_suspended = false;

	return SM_SUCCESS;

exit_hw_setup:
	return res;
}



static int mpu65xx_if_enable(struct invsens_driver_t *drv,
				  u32 func_mask, bool enable,
				  struct invsens_sm_ctrl_t *sm_ctrl)
{
	int res = 0;
	struct mpu65xx_ctrl_t *dctl;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	/*user configuration */
	if (sm_ctrl) {
		memcpy(&dctl->sm_ctrl, sm_ctrl, sizeof(dctl->sm_ctrl));
		INVSENS_LOGD("%s : mask=%d\n", __func__,
			     dctl->sm_ctrl.pysical_mask);
	}

	res = mpu65xx_enable(dctl, func_mask, enable);

	if (dctl->enabled_mask)
		drv->is_activated = true;
	else
		drv->is_activated = false;

	return res;
}

static int mpu65xx_if_set_delay(struct invsens_driver_t *drv,
				     u32 func_mask, long delay)
{
	int res = 0;
	struct mpu65xx_ctrl_t *dctl;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	res = mpu65xx_set_delay(dctl, func_mask, delay, false);
	return res;
}

static int mpu65xx_if_read(struct invsens_driver_t *drv,
				struct invsens_data_list_t *buffer)
{
	int res = 0;
	struct mpu65xx_ctrl_t *dctl = NULL;


	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);
	res = mpu65xx_read(dctl, buffer);
	return res;
}


static int mpu65xx_if_terminate(struct invsens_driver_t *drv)
{
	int res = 0;
	struct mpu65xx_ctrl_t *dctl;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	kzfree(dctl);

	drv->user_data = NULL;

	return res;
}

static int mpu65xx_if_suspend(struct invsens_driver_t *drv)
{
	struct mpu65xx_ctrl_t *dctl;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);


	return mpu65xx_suspend(dctl);
}

static int mpu65xx_if_resume(struct invsens_driver_t *drv)
{
	struct mpu65xx_ctrl_t *dctl;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);


	return mpu65xx_resume(dctl);
}



static int mpu65xx_if_ioctl(struct invsens_driver_t *drv, u32 cmd,
				 long lparam, void *vparam)
{
	int res = SM_SUCCESS;
	struct mpu65xx_ctrl_t *dctl;
	struct invsens_ioctl_param_t *ioctl = vparam;

	INV_DBG_FUNC_NAME;

	dctl = INVSENS_DRV_USER_DATA(drv);

	switch (cmd) {
	case INV_IOCTL_GYRO_SW_SELFTEST:
		{
			long scaled_bias[3];
			unsigned long scaled_rms[3];
			int packet_count;
			int st_result = 0;
			res = mpu65xx_sw_selftest(dctl,
				scaled_bias, scaled_rms, &packet_count);
			/*30dps threhold*/
			if (scaled_bias[0] > 30*DEF_ST_PRECISION ||
				scaled_bias[1] > 30*DEF_ST_PRECISION ||
				scaled_bias[2] > 30*DEF_ST_PRECISION)
				st_result = 0; /*fail*/
			else
				st_result = 1; /*success*/

			if (ioctl) {
				ioctl->swst_gyro.result = st_result;
				ioctl->swst_gyro.bias[0] = scaled_bias[0];
				ioctl->swst_gyro.bias[1] = scaled_bias[1];
				ioctl->swst_gyro.bias[2] = scaled_bias[2];
				ioctl->swst_gyro.rms[0] = scaled_rms[0];
				ioctl->swst_gyro.rms[1] = scaled_rms[1];
				ioctl->swst_gyro.rms[2] = scaled_rms[2];
				ioctl->swst_gyro.packet_cnt = packet_count;
			}
		}
		return SM_IOCTL_HANDLED;

	case INV_IOCTL_GYRO_HW_SELFTEST:
		{
		int gyro_st_ratio[THREE_AXIS], accel_st_ratio[THREE_AXIS];

		res = mpu65xx_hw_selftest(dctl, accel_st_ratio, gyro_st_ratio);

			if (ioctl) {
				ioctl->hwst_gyro.result = res & 0x1;
				ioctl->hwst_gyro.ratio[0] = gyro_st_ratio[0];
				ioctl->hwst_gyro.ratio[1] = gyro_st_ratio[1];
				ioctl->hwst_gyro.ratio[2] = gyro_st_ratio[2];
			}
		}
		return SM_IOCTL_HANDLED;
	case INV_IOCTL_ACCEL_HW_SELFTEST:
		{
			int gyro_st_ratio[THREE_AXIS];
			int accel_st_ratio[THREE_AXIS];

			res = mpu65xx_hw_selftest(dctl,
				accel_st_ratio, gyro_st_ratio);

			if (ioctl) {
				ioctl->hwst_accel.result = (res >> 1) & 0x1;
				ioctl->hwst_accel.ratio[0] = accel_st_ratio[0];
				ioctl->hwst_accel.ratio[1] = accel_st_ratio[1];
				ioctl->hwst_accel.ratio[2] = accel_st_ratio[2];
			}
		}
		return SM_IOCTL_HANDLED;
	case INV_IOCTL_GET_ACCEL_RAW:
		{
			short accel_raw[THREE_AXIS];
			mpu65xx_read_accel_raw(dctl, accel_raw);
			if (ioctl) {
				ioctl->accel_raw.raw[0] = accel_raw[0];
				ioctl->accel_raw.raw[1] = accel_raw[1];
				ioctl->accel_raw.raw[2] = accel_raw[2];
			}
		}
		return SM_IOCTL_HANDLED;
	case INV_IOCTL_GET_GYRO_RAW:
		{
			short gyro_raw[THREE_AXIS];
			mpu65xx_read_gyro_raw(dctl, gyro_raw);
			if (ioctl) {
				ioctl->gyro_raw.raw[0] = gyro_raw[0];
				ioctl->gyro_raw.raw[1] = gyro_raw[1];
				ioctl->gyro_raw.raw[2] = gyro_raw[2];
			}
		}
		return SM_IOCTL_HANDLED;

	case INV_IOCTL_S_GYRO_SELFTEST:
		{
			long scaled_bias[3];
			unsigned long scaled_rms[3];
			int packet_count;
			int st_result = 0, hwst_result;
			int gyro_st_ratio[THREE_AXIS];
			int accel_st_ratio[THREE_AXIS];

			/*sw selftest*/
			res = mpu65xx_sw_selftest(dctl,
				scaled_bias, scaled_rms, &packet_count);
			/*30dps threhold*/
			if (scaled_bias[0] > 30*DEF_ST_PRECISION ||
				scaled_bias[1] > 30*DEF_ST_PRECISION ||
				scaled_bias[2] > 30*DEF_ST_PRECISION)
				st_result = 0; /*fail*/
			else
				st_result = 1; /*success*/
			hwst_result = mpu65xx_hw_selftest(dctl,
				accel_st_ratio, gyro_st_ratio);
			if (ioctl) {
				ioctl->s_gyro_selftest.result =
					(!(hwst_result & 0x1) | !st_result);
				ioctl->s_gyro_selftest.gyro_ratio[0] =
					gyro_st_ratio[0];
				ioctl->s_gyro_selftest.gyro_ratio[1] =
					gyro_st_ratio[1];
				ioctl->s_gyro_selftest.gyro_ratio[2] =
					gyro_st_ratio[2];
				ioctl->s_gyro_selftest.gyro_bias[0] =
					scaled_bias[0];
				ioctl->s_gyro_selftest.gyro_bias[1] =
					scaled_bias[1];
				ioctl->s_gyro_selftest.gyro_bias[2] =
					scaled_bias[2];
				ioctl->s_gyro_selftest.gyro_rms[0] =
					scaled_rms[0];
				ioctl->s_gyro_selftest.gyro_rms[1] =
					scaled_rms[1];
				ioctl->s_gyro_selftest.gyro_rms[2] =
					scaled_rms[2];
				ioctl->s_gyro_selftest.packet_cnt =
					packet_count;
			}
		}
		return SM_IOCTL_HANDLED;

	case INV_IOCTL_S_ACCEL_SELFTEST:
		{
			int gyro_st_ratio[THREE_AXIS];
			int accel_st_ratio[THREE_AXIS];

			res = mpu65xx_hw_selftest(dctl,
				accel_st_ratio, gyro_st_ratio);

			if (ioctl) {
				ioctl->s_accel_selftest.result =
					((res >> 1) & 0x1);
				ioctl->s_accel_selftest.accel_ratio[0] =
					accel_st_ratio[0];
				ioctl->s_accel_selftest.accel_ratio[1] =
					accel_st_ratio[1];
				ioctl->s_accel_selftest.accel_ratio[2] =
					accel_st_ratio[2];
			}
		}
		return SM_IOCTL_HANDLED;

	case INV_IOCTL_SET_ACCEL_BIAS:
		memcpy(dctl->accel_bias, ioctl->accel_bias.bias,
			sizeof(dctl->accel_bias));

		return SM_IOCTL_HANDLED;

	case INV_IOCTL_GET_ACCEL_BIAS:
		memcpy(ioctl->accel_bias.bias, dctl->accel_bias,
			sizeof(dctl->accel_bias));

		return SM_IOCTL_HANDLED;

	case INV_IOCTL_DO_ACCEL_CAL:
		mpu65xx_do_accel_calib(dctl, ioctl->accel_bias.bias);

		return SM_IOCTL_HANDLED;

	case INV_IOCTL_SET_WOM_THRESH:
		{
			u8 wom = ioctl->data[0];
			dctl->reg.wom_thres = wom;
		}
		return SM_IOCTL_HANDLED;

	case INV_IOCTL_GET_TEMP:
		mpu65xx_get_temperature(dctl,
			&ioctl->temperature.temp);

		return SM_IOCTL_HANDLED;

	case INV_IOCTL_RESET_FIFO:
		mpu65xx_reset_fifo(dctl);

		return SM_IOCTL_HANDLED;

	}

	return SM_IOCTL_NOTHANDLED;
}



static struct invsens_driver_t mpu65xx_handler = {
	.driver_id = INV_DRIVER_MPU65XX,
	.driver_layer = INV_DRIVER_LAYER_NATIVE,
	.func_mask = (1 << INV_FUNC_ACCEL) | (1 << INV_FUNC_GYRO)
		| (1 << INV_FUNC_MOTION_INTERRUPT)
		| (1 << INV_FUNC_FT_SENSOR_ON),

	.init = mpu65xx_if_init,
	.enable = mpu65xx_if_enable,
	.set_delay = mpu65xx_if_set_delay,
	.terminate = mpu65xx_if_terminate,
	.read = mpu65xx_if_read,
	.suspend = mpu65xx_if_suspend,
	.resume = mpu65xx_if_resume,
	.ioctl = mpu65xx_if_ioctl,
	.user_data = NULL,
};

int invsens_load_mpu65xx(struct invsens_driver_t **driver,
	struct invsens_board_cfg_t *board_cfg)
{
	int res = 0;
	struct mpu65xx_ctrl_t *dctl = NULL;

	INV_DBG_FUNC_NAME;

	if (!driver) {
		res = -EINVAL;
		goto error_case;
	}

	dctl = kzalloc(sizeof(struct mpu65xx_ctrl_t), GFP_KERNEL);
	if (!dctl) {
		res = -ENOMEM;
		goto error_case;
	}

	dctl->i2c_handle = board_cfg->i2c_handle;
	dctl->i2c_addr = board_cfg->i2c_addr;

	if (board_cfg->platform_data)
		memcpy(dctl->orientation,
		       board_cfg->platform_data->orientation,
		       sizeof(dctl->orientation));

	mpu65xx_handler.user_data = dctl;

	*driver = &mpu65xx_handler;

	return 0;
error_case:
	return res;
}
