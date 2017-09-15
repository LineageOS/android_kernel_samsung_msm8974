/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	$
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "inv_mpu_iio.h"
#include "mpu6500_selftest.h"

#define VERBOSE_OUT 1

#define X (0)
#define Y (1)
#define Z (2)

/*--- Test parameters defaults. See set_test_parameters for more details ---*/

#define DEF_MPU_ADDR             (0x68)	/* I2C address of the mpu     */

#define DEF_GYRO_FULLSCALE       (2000)	/* gyro full scale dps        */
#define DEF_GYRO_SENS            (32768 / DEF_GYRO_FULLSCALE)
    /* gyro sensitivity LSB/dps   */
#define DEF_PACKET_THRESH        (75)	/* 75 ms / (1ms / sample) OR
					   600 ms / (8ms / sample)   */
#define DEF_TOTAL_TIMING_TOL     (3)	/* 3% = 2 pkts + 1% proc tol. */

#define DEF_BIAS_THRESH_SELF     (20)	/* dps */
#define DEF_BIAS_THRESH_CAL      (20)

#define DEF_BIAS_LSB_THRESH_SELF (DEF_BIAS_THRESH_SELF*DEF_GYRO_SENS)
#define DEF_BIAS_LSB_THRESH_CAL     (DEF_BIAS_THRESH_CAL*DEF_GYRO_SENS)

/* 0.4 dps-rms in LSB-rms	  */
#define DEF_RMS_THRESH_SELF     (5)	/* dps : spec  is 0.4dps_rms */
#define DEF_RMS_LSB_THRESH_SELF (DEF_RMS_THRESH_SELF*DEF_GYRO_SENS)

#define DEF_TESTS_PER_AXIS       (1)	/* num of periods used to test
					   each axis */
#define DEF_N_ACCEL_SAMPLES      (20)	/* num of accel samples to
					   average from, if applic.   */
#define ML_INIT_CAL_LEN          (36)	/* length in bytes of
					   calibration data file      */
#define DEF_PERIOD_SELF          (75)	/* ms of time, self test */
#define DEF_PERIOD_CAL           (600)	/* ms of time, full cal */

#define DEF_SCALE_FOR_FLOAT	(1000)
#define DEF_RMS_SCALE_FOR_RMS	(10000)
#define DEF_SQRT_SCALE_FOR_RMS	(100)

#define FIFO_PACKET_SIZE	6
#define GYRO_PACKET_THRESH	50
#define GYRO_MAX_PACKET_THRESH	75
#define BIT_GYRO_FIFO_EN	(BIT_XG_FIFO_EN|BIT_YG_FIFO_EN|BIT_ZG_FIFO_EN)
#define BIT_FIFO_DIS		0x0
#define BIT_XG_FIFO_EN		0x40
#define BIT_YG_FIFO_EN		0x20
#define BIT_ZG_FIFO_EN		0x10
#define GYRO_WAIT_TIME		GYRO_PACKET_THRESH
#define GYRO_THRESH		3
#define THREE_AXIS		3

/* HW self test */
#define BYTES_PER_SENSOR	6
#define DEF_ST_STABLE_TIME	200
#define DEF_GYRO_WAIT_TIME	51
#define DEF_ST_PRECISION	1000
#define BIT_ACCEL_OUT		0x08
#define BITS_GYRO_OUT		0x70
#define FIFO_COUNT_BYTE		2
#define BITS_SELF_TEST_EN	0xE0

#define INIT_SELFTEST_SAMPLES		50
#define INIT_SELFTEST_THRESHOLD	14
#define DEF_ST_TRY_TIMES	2

#define REG_ST_GCT_X		0xD
#define DEF_GYRO_CT_SHIFT_DELTA	500
/* gyroscope Coriolis self test min and max bias shift (dps) */
#define DEF_GYRO_CT_SHIFT_MIN	10
#define DEF_GYRO_CT_SHIFT_MAX	105
#define DEF_ST_OTP0_THRESH	60
#define DEF_ST_ABS_THRESH	20
#define DEF_ST_TOR		2
#define DEF_GYRO_SCALE		131
#define DEF_GYRO_SELFTEST_DPS	250
#define DEF_SELFTEST_GYRO_SENS	(32768 / 250)

static const int gyro_6500_st_tb[255] = {
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


/*
    Types
*/
struct mpu6500_selftest_info {
	int gyro_sens;
	int gyro_fs;
	int packet_thresh;
	int total_timing_tol;
	int bias_thresh;
	int rms_thresh;
	unsigned int tests_per_axis;
	unsigned short accel_samples;
};

struct mpu6500_selftest {
	unsigned char pwm_mgmt[2];
	unsigned char smplrt_div;
	unsigned char user_ctrl;
	unsigned char config;
	unsigned char gyro_config;
	unsigned char int_enable;
};

/*
    Global variables
*/

static struct mpu6500_selftest mpu6500_selftest;

static int mpu6500_backup_register(struct inv_mpu_state *st)
{
	int result = 0;

	result =
	    inv_i2c_read(st, MPUREG_PWR_MGMT_1,
				 2, mpu6500_selftest.pwm_mgmt);
	if (result)
		return result;

	result =
	    inv_i2c_read(st, MPUREG_CONFIG,
				 1, &mpu6500_selftest.config);
	if (result)
		return result;

	result =
	    inv_i2c_read(st, MPUREG_GYRO_CONFIG,
				 1, &mpu6500_selftest.gyro_config);
	if (result)
		return result;

	result =
	    inv_i2c_read(st, MPUREG_USER_CTRL,
				 1, &mpu6500_selftest.user_ctrl);
	if (result)
		return result;

	result =
	    inv_i2c_read(st, MPUREG_INT_ENABLE,
				 1, &mpu6500_selftest.int_enable);
	if (result)
		return result;

	result =
	    inv_i2c_read(st, MPUREG_SMPLRT_DIV,
				 1, &mpu6500_selftest.smplrt_div);
	if (result)
		return result;

	return result;
}

static int mpu6500_recover_register(struct inv_mpu_state *st)
{
	int result = 0;

	result =
	    inv_i2c_single_write(st, MPUREG_CONFIG,
					 mpu6500_selftest.config);
	if (result)
		return result;

	result =
	    inv_i2c_single_write(st, MPUREG_GYRO_CONFIG,
					 mpu6500_selftest.gyro_config);
	if (result)
		return result;

	result =
	    inv_i2c_single_write(st, MPUREG_USER_CTRL,
					 mpu6500_selftest.user_ctrl);
	if (result)
		return result;

	result =
	    inv_i2c_single_write(st, MPUREG_SMPLRT_DIV,
					 mpu6500_selftest.smplrt_div);
	if (result)
		return result;

	result =
	    inv_i2c_single_write(st, MPUREG_INT_ENABLE,
					 mpu6500_selftest.int_enable);
	if (result)
		return result;

	result =
	    inv_i2c_single_write(st, MPUREG_PWR_MGMT_2,
					 mpu6500_selftest.pwm_mgmt[1]);
	if (result)
		return result;

	result =
	    inv_i2c_single_write(st, MPUREG_PWR_MGMT_1,
					 mpu6500_selftest.pwm_mgmt[0]);
	if (result)
		return result;


	return result;
}

u32 mpu6500_selftest_sqrt(u32 sqsum)
{
	u32 sq_rt;

	int g0, g1, g2, g3, g4;
	int seed;
	int next;
	int step;

	g4 = sqsum / 100000000;
	g3 = (sqsum - g4 * 100000000) / 1000000;
	g2 = (sqsum - g4 * 100000000 - g3 * 1000000) / 10000;
	g1 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000) / 100;
	g0 = (sqsum - g4 * 100000000 - g3 * 1000000 - g2 * 10000 - g1 * 100);

	next = g4;
	step = 0;
	seed = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = seed * 10000;
	next = (next - (seed * step)) * 100 + g3;

	step = 0;
	seed = 2 * seed * 10;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 1000;
	next = (next - seed * step) * 100 + g2;
	seed = (seed + step) * 10;
	step = 0;
	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 100;
	next = (next - seed * step) * 100 + g1;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step * 10;
	next = (next - seed * step) * 100 + g0;
	seed = (seed + step) * 10;
	step = 0;

	while (((seed + 1) * (step + 1)) <= next) {
		step++;
		seed++;
	}

	sq_rt = sq_rt + step;

	return sq_rt;
}

short mpu_big8_to_int16(unsigned char *big8)
{
	short x;
	x = ((short)big8[0] << 8) | ((short)big8[1]);
	return x;
}

int mpu6500_selftest_run(struct inv_mpu_state *st,
			 int packet_cnt[3],
			 int gyro_bias[3],
			 int gyro_rms[3],
			 int gyro_lsb_bias[3])
{
	int ret_val = 0;
	int result;
	int packet_count;
	long avg[3]={0};
	long rms[3]={0};
	int i, j;
	unsigned char regs[7] = {0};
	unsigned char data[FIFO_PACKET_SIZE * 2]={0}, read_data[2];
	int gyro_data[3][GYRO_MAX_PACKET_THRESH]={{0},};
	short fifo_cnt;
	int gyro_avg_tmp[3]={0};

	struct mpu6500_selftest_info test_setup = {
		DEF_GYRO_SENS, DEF_GYRO_FULLSCALE, DEF_PACKET_THRESH,
		DEF_TOTAL_TIMING_TOL, (int)DEF_BIAS_THRESH_SELF,
		DEF_RMS_LSB_THRESH_SELF * DEF_RMS_LSB_THRESH_SELF,
		/* now obsolete - has no effect */
		DEF_TESTS_PER_AXIS, DEF_N_ACCEL_SAMPLES
	};

	char a_name[3][2] = { "X", "Y", "Z" };

	/*backup registers */
	result = mpu6500_backup_register(st);
	if (result) {
		printk(KERN_ERR "register backup error=%d", result);
		return result;
	}

	if (mpu6500_selftest.pwm_mgmt[0] & 0x40) {
		result = inv_i2c_single_write(st, MPUREG_PWR_MGMT_1, 0x00);
		if (result) {
			printk(KERN_INFO "init PWR_MGMT error=%d", result);
			return result;
		}
	}

	regs[0] = mpu6500_selftest.pwm_mgmt[1] & ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	result = inv_i2c_single_write(st, MPUREG_PWR_MGMT_2, regs[0]);

	result = inv_i2c_single_write(st, MPUREG_INT_ENABLE, 0);
	if (result)
		return result;

	/* disable the sensor output to FIFO */
	result = inv_i2c_single_write(st, MPUREG_FIFO_EN, 0);
	if (result)
		return result;

	/* make sure the DMP is disabled first */
	result = inv_i2c_single_write(st, MPUREG_USER_CTRL, 0x00);
	if (result) {
		printk(KERN_INFO "DMP disable error=%d", result);
		return result;
	}

	/* clear FIFO */
	result = inv_i2c_single_write(st, MPUREG_USER_CTRL, BIT_FIFO_RST);
	if (result)
		return result;

	/* sample rate *//* = 1ms */
	result = inv_i2c_single_write(st, MPUREG_SMPLRT_DIV, 0x00);
	if (result)
		return result;

	test_setup.bias_thresh = DEF_BIAS_LSB_THRESH_SELF;

	regs[0] = 0x03;		/* filter = 42Hz, analog_sample rate = 1 KHz */
	switch (test_setup.gyro_fs) {
	case 2000:
		regs[0] |= 0x18;
		break;
	case 1000:
		regs[0] |= 0x10;
		break;
	case 500:
		regs[0] |= 0x08;
		break;
	case 250:
	default:
		regs[0] |= 0x00;
		break;
	}
	result = inv_i2c_single_write(st, MPUREG_CONFIG, regs[0]);
	if (result)
		return result;

	switch (test_setup.gyro_fs) {
	case 2000:
		regs[0] = 0x03;
		break;
	case 1000:
		regs[0] = 0x02;
		break;
	case 500:
		regs[0] = 0x01;
		break;
	case 250:
	default:
		regs[0] = 0x00;
		break;
	}
	result = inv_i2c_single_write(st, MPUREG_GYRO_CONFIG, regs[0] << 3);
	if (result)
		return result;

	// Wait time
//	msleep(GYRO_WAIT_TIME);
	mdelay(200);

	// Enable FIFO
	result = inv_i2c_single_write(st, MPUREG_USER_CTRL, BIT_FIFO_EN);
	if (result)
	  return result;

	// Enable gyro output to FIFO
	result = inv_i2c_single_write(st, MPUREG_FIFO_EN, BIT_GYRO_FIFO_EN);
	if (result)
	  return result;

	// Wait time
	mdelay(GYRO_WAIT_TIME);


	// Stop gyro FIFO
	result = inv_i2c_single_write(st, MPUREG_FIFO_EN, BIT_FIFO_DIS);
	if (result)
	  return result;

	// Read FIFO count
	result = inv_i2c_read(st, MPUREG_FIFO_COUNTH, 2, read_data);
	if (result)
		return result;

	fifo_cnt = be16_to_cpup((__be16 *)(&read_data[0]));

	packet_count = fifo_cnt != 0 ? (fifo_cnt/FIFO_PACKET_SIZE) : 1;

	if(packet_count > GYRO_PACKET_THRESH)
		packet_count = GYRO_PACKET_THRESH;

	// Check packet count
	if((abs(packet_count - GYRO_PACKET_THRESH) > GYRO_THRESH) && (packet_count < GYRO_PACKET_THRESH)) {
		printk("\r\n Gyro Packet counter Error: %d \r\n",packet_count);
	  return (ret_val |= 1);
	}

	printk("\r\n Gyro Packet counter : %d \r\n",packet_count);

	for (i = 0; i < packet_count; i++) {
	   /* getting FIFO data */
	     result = inv_i2c_read(st, MPUREG_FIFO_R_W, FIFO_PACKET_SIZE, data);
	     if (result)
		return result;

	   for (j = 0; j < THREE_AXIS; j++) {
		   gyro_data[j][i] = (int)mpu_big8_to_int16((&data[2*j]));
		   gyro_avg_tmp[j] += gyro_data[j][i];
		   avg[j] = (long)gyro_avg_tmp[j];
		   avg[j] /= packet_count;
	   }
	}



	printk(KERN_INFO "bias : %+8ld %+8ld %+8ld (LSB)\n", avg[X], avg[Y], avg[Z]);

	gyro_bias[X] = (int)((avg[X] * DEF_SCALE_FOR_FLOAT) / DEF_GYRO_SENS);
	gyro_bias[Y] = (int)((avg[Y] * DEF_SCALE_FOR_FLOAT) / DEF_GYRO_SENS);
	gyro_bias[Z] = (int)((avg[Z] * DEF_SCALE_FOR_FLOAT) / DEF_GYRO_SENS);
	gyro_lsb_bias[X] = (int)avg[X];
	gyro_lsb_bias[Y] = (int)avg[Y];
	gyro_lsb_bias[Z] = (int)avg[Z];

	if (VERBOSE_OUT) {
		printk(KERN_INFO
		       "abs bias : %+8d.%03d   %+8d.%03d  %+8d.%03d (dps)\n",
		       (int)abs(gyro_bias[X]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[X]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Y]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Y]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Z]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_bias[Z]) % DEF_SCALE_FOR_FLOAT);
	}

	for (j = 0; j < 3; j++) {
		if (abs(avg[j]) > test_setup.bias_thresh) {
			printk(KERN_INFO
			       "%s-Gyro bias (%ld) exceeded threshold "
			       "(threshold = %d LSB)\n", a_name[j], avg[j],
			       test_setup.bias_thresh);
			ret_val |= 1 << (3 + j);
		}
	}

	/* 3rd, check RMS for dead gyros
	   If any of the RMS noise value returns zero,
	   then we might have dead gyro or FIFO/register failure,
	   the part is sleeping, or the part is not responsive */
	for (i = 0, rms[X] = 0, rms[Y] = 0, rms[Z] = 0; i < packet_count; i++) {
		rms[X] += (long)(gyro_data[0][i] - avg[X]) * (gyro_data[0][i] - avg[X]);
		rms[Y] += (long)(gyro_data[1][i] - avg[Y]) * (gyro_data[1][i] - avg[Y]);
		rms[Z] += (long)(gyro_data[2][i] - avg[Z]) * (gyro_data[2][i] - avg[Z]);
	}

	if (rms[X] == 0 || rms[Y] == 0 || rms[Z] == 0)
		ret_val |= 1 << 6;

	if (VERBOSE_OUT) {
		printk(KERN_INFO "RMS ^ 2 : %+8ld %+8ld %+8ld\n",
		       (long)rms[X] / packet_count,
		       (long)rms[Y] / packet_count, (long)rms[Z] / packet_count);
	}

	{
		int dps_rms[3] = { 0 };
		u32 tmp;
		int i = 0;

		for (j = 0; j < 3; j++) {
			if (rms[j] / packet_count > test_setup.rms_thresh) {
				printk(KERN_INFO
				       "%s-Gyro rms (%ld) exceeded threshold "
				       "(threshold = %d LSB)\n", a_name[j],
				       rms[j] / packet_count,
				       test_setup.rms_thresh);
				ret_val |= 1 << (7 + j);
			}
		}

		for (i = 0; i < 3; i++) {
			if (rms[i] > 10000) {
				tmp = ((u32) (rms[i] / packet_count)) * DEF_RMS_SCALE_FOR_RMS;
			} else {
				tmp = ((u32) (rms[i] * DEF_RMS_SCALE_FOR_RMS)) / packet_count;
			}

			if (rms[i] < 0)
				tmp = 1 << 31;

			dps_rms[i] = mpu6500_selftest_sqrt(tmp) / DEF_GYRO_SENS;

			gyro_rms[i] = dps_rms[i] * DEF_SCALE_FOR_FLOAT / DEF_SQRT_SCALE_FOR_RMS;
		}

		printk(KERN_INFO
		       "RMS : %+8d.%03d	 %+8d.%03d  %+8d.%03d (dps)\n",
		       (int)abs(gyro_rms[X]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[X]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Y]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Y]) % DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Z]) / DEF_SCALE_FOR_FLOAT,
		       (int)abs(gyro_rms[Z]) % DEF_SCALE_FOR_FLOAT);
	}

	/*recover registers */
	result = mpu6500_recover_register(st);
	if (result) {
		printk(KERN_ERR "register recovering error=%d", result);
		return result;
	}
	return ret_val;
}

static int mpu6500_do_powerup(struct inv_mpu_state *st)
{
	int result = 0;
	char reg;

	inv_i2c_single_write(st, MPUREG_PWR_MGMT_1, 0x1);

	mdelay(20);

	inv_i2c_read(st, MPUREG_PWR_MGMT_2, 1, &reg);

	reg &= ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	inv_i2c_single_write(st, MPUREG_PWR_MGMT_2, reg);

	return result;
}

static int mpu6500_do_test(struct inv_mpu_state *st, int self_test_flag, int *gyro_result)
{
	int result, i, j, packet_size;
	u8 data[BYTES_PER_SENSOR * 2], d;
	int fifo_count, packet_count, ind, s;

	packet_size = BYTES_PER_SENSOR;

	result = inv_i2c_single_write(st, MPUREG_INT_ENABLE, 0);
	if (result)
		return result;
	/* disable the sensor output to FIFO */
	result = inv_i2c_single_write(st, MPUREG_FIFO_EN, 0);
	if (result)
		return result;
	/* disable fifo reading */
	result = inv_i2c_single_write(st, MPUREG_USER_CTRL, 0);
	if (result)
		return result;
	/* clear FIFO */
	result = inv_i2c_single_write(st, MPUREG_USER_CTRL, BIT_FIFO_RST);
	if (result)
		return result;
	/* setup parameters */
	result = inv_i2c_single_write(st, MPUREG_CONFIG, MPU_FILTER_184HZ);
	if (result)
		return result;
	result = inv_i2c_single_write(st, MPUREG_SMPLRT_DIV, 0x0);
	if (result)
		return result;
	result = inv_i2c_single_write(st, MPUREG_GYRO_CONFIG, self_test_flag | (MPU_FS_250DPS << 3));
	if (result)
		return result;

	/* wait for the output to get stable */
	mdelay(DEF_ST_STABLE_TIME);

	/* enable FIFO reading */
	result = inv_i2c_single_write(st,
			MPUREG_USER_CTRL, BIT_FIFO_EN);
	if (result)
		return result;
	/* enable sensor output to FIFO */
	d = BITS_GYRO_OUT;
	result = inv_i2c_single_write(st, MPUREG_FIFO_EN, d);
	if (result)
		return result;

	for (i = 0; i < THREE_AXIS; i++)
		gyro_result[i] = 0;

	s = 0;

	while (s < INIT_SELFTEST_SAMPLES) {
		mdelay(DEF_GYRO_WAIT_TIME);
		/* stop sending data to FIFO */
		result = inv_i2c_single_write(st, MPUREG_FIFO_EN, 0);
		if (result)
			return result;

		result = inv_i2c_read(st,
			MPUREG_FIFO_COUNTH, FIFO_COUNT_BYTE, data);

		if (result)
			return result;

		fifo_count = be16_to_cpup((__be16 *)(&data[0]));
		packet_count = fifo_count / packet_size;
		result = inv_i2c_read(st, MPUREG_FIFO_R_W,
			packet_size, data);

		if (result)
			return result;

		i = 0;

		while ((i < packet_count) && (s < INIT_SELFTEST_SAMPLES)) {
			result = inv_i2c_read(st, MPUREG_FIFO_R_W,
				packet_size, data);
			if (result)
				return result;

			ind = 0;

			for (j = 0; j < THREE_AXIS; j++)
			gyro_result[j] +=
			(short)be16_to_cpup( (__be16 *)(&data[ind + 2 * j]));

			s++;
			i++;
		}
	}

	for (j = 0; j < THREE_AXIS; j++) {
		gyro_result[j] = gyro_result[j]/s;
		gyro_result[j] *= DEF_ST_PRECISION;
	}

	return 0;
}

static int mpu6500_gyro_self_test(struct inv_mpu_state *st,
		int *reg_avg, int *st_avg, int *ratio)
{
	int result;
	int ret_val;
	int ct_shift_prod[3], st_shift_cust[3], st_shift_ratio[3], i;
	u8 regs[3];

	ret_val = 0;

	result = inv_i2c_read(st, MPUREG_SELF_TEST_X_GYRO, 3, regs);
	if (result)
		return result;

	for (i = 0; i < 3; i++) {
		if (regs[i] != 0)
			ct_shift_prod[i] = gyro_6500_st_tb[regs[i] - 1];
		else
			ct_shift_prod[i] = 0;
	}

	pr_err("reg_bias : %d, %d, %d \n", reg_avg[0], reg_avg[1], reg_avg[2]);
	pr_err("st_avg : %d, %d, %d \n", st_avg[0], st_avg[1], st_avg[2]);

	for (i = 0; i < 3; i++) {
		st_shift_cust[i] = abs(reg_avg[i] - st_avg[i]);
		if (ct_shift_prod[i]) {

			st_shift_ratio[i] =
				abs((st_shift_cust[i] / ct_shift_prod[i])
					- DEF_ST_PRECISION);

			ratio[i] = st_shift_ratio[i];
			if (st_shift_ratio[i] > DEF_GYRO_CT_SHIFT_DELTA)
				ret_val |= 1 << i;
		} else {
			if (st_shift_cust[i] <
				DEF_ST_PRECISION * DEF_GYRO_CT_SHIFT_MIN *
				DEF_SELFTEST_GYRO_SENS)
				ret_val |= 1 << i;
			if (st_shift_cust[i] > 
				DEF_ST_PRECISION * DEF_GYRO_CT_SHIFT_MAX *
				DEF_SELFTEST_GYRO_SENS)
				ret_val |= 1 << i;
		}
	}

	pr_err("ct_shift_prod : %d %d %d\n", ct_shift_prod[0],
			ct_shift_prod[1], ct_shift_prod[2]);

	pr_err("st_shift_cust : %d %d %d\n", st_shift_cust[0],
			st_shift_cust[1], st_shift_cust[2]);

	pr_err("st_shift_ratio : %d %d %d\n", st_shift_ratio[0],
			st_shift_ratio[1], st_shift_ratio[2]);

	pr_err("%s, ret_val = %d\n", __func__, ret_val);

	return ret_val;
}


int mpu6500_gyro_hw_self_check(struct inv_mpu_state *st, int ratio[3])
{
	int result;
	int gyro_bias_st[THREE_AXIS], gyro_bias_regular[THREE_AXIS];
	int test_times;
	int gyro_result;

	/*backup registers */
	result = mpu6500_backup_register(st);
	if (result) {
		pr_err("register backup error=%d", result);
		goto test_fail;
	}

	/*power-up*/
	mpu6500_do_powerup(st);

	/*get regular bias*/
	test_times = DEF_ST_TRY_TIMES;
	while (test_times > 0) {
		result = mpu6500_do_test(st, 0, gyro_bias_regular);
		if (result == -EAGAIN)
			test_times--;
		else
			test_times = 0;
	}
	if (result)
		goto test_fail;

	/*get st bias*/
	test_times = DEF_ST_TRY_TIMES;
	while (test_times > 0) {
		result = mpu6500_do_test(st,
			BITS_SELF_TEST_EN, gyro_bias_st);
		if (result == -EAGAIN)
			test_times--;
		else
			test_times = 0;
	}
	if (result)
		goto test_fail;

	gyro_result = mpu6500_gyro_self_test(st, gyro_bias_regular,
		gyro_bias_st, ratio);
	result = gyro_result;

test_fail:
	/*recover registers */
	if (mpu6500_recover_register(st)) {
		pr_err("register recovering error\n");
		if (mpu6500_recover_register(st))
			pr_err("register recovering error2\n");

	}

	pr_err("%s, gyro hw result = %d\n", __func__, result);

	return result;
}
