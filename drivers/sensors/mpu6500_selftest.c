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
#include <linux/input.h>
#include <linux/input-polldev.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "./mpu6500_input.h"
#include "./mpu6500_selftest.h"

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
#define THREE_AXIS		3
#define DEF_GYRO_WAIT_TIME	51
#define DEF_ST_PRECISION	1000
#define BIT_ACCEL_OUT		0x08
#define BITS_GYRO_OUT		0x70
#define FIFO_COUNT_BYTE		2
#define BITS_SELF_TEST_EN	0xE0

#define INIT_ST_SAMPLES		50
#define INIT_ST_THRESHOLD	14
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

static const int gyro_6500_st_tb[256] = {
	2621, 2648, 2674, 2701, 2728, 2755, 2783, 2811,
	2839, 2867, 2896, 2925, 2954, 2983, 3013, 3043,
	3074, 3105, 3136, 3167, 3199, 3231, 3263, 3296,
	3329, 3362, 3395, 3429, 3464, 3498, 3533, 3569,
	3604, 3640, 3677, 3714, 3751, 3788, 3826, 3864,
	3903, 3942, 3981, 4021, 4061, 4102, 4143, 4185,
	4226, 4269, 4311, 4354, 4398, 4442, 4486, 4531,
	4577, 4622, 4669, 4715, 4762, 4810, 4858, 4907,
	4956, 5005, 5055, 5106, 5157, 5209, 5261, 5313,
	5366, 5420, 5474, 5529, 5584, 5640, 5696, 5753,
	5811, 5869, 5928, 5987, 6047, 6107, 6168, 6230,
	6292, 6355, 6419, 6483, 6548, 6613, 6680, 6746,
	6814, 6882, 6951, 7020, 7091, 7161, 7233, 7305,
	7378, 7452, 7527, 7602, 7678, 7755, 7832, 7911,
	7990, 8070, 8150, 8232, 8314, 8397, 8481, 8566,
	8652, 8738, 8826, 8914, 9003, 9093, 9184, 9276,
	9369, 9462, 9557, 9653, 9749, 9847, 9945, 10044,
	10145, 10246, 10349, 10452, 10557, 10662, 10769, 10877,
	10985, 11095, 11206, 11318, 11432, 11546, 11661, 11778,
	11896, 12015, 12135, 12256, 12379, 12502, 12627, 12754,
	12881, 13010, 13140, 13272, 13404, 13538, 13674, 13810,
	13949, 14088, 14229, 14371, 14515, 14660, 14807, 14955,
	15104, 15255, 15408, 15562, 15718, 15875, 16034, 16194,
	16356, 16519, 16685, 16851, 17020, 17190, 17362, 17536,
	17711, 17888, 18067, 18248, 18430, 18614, 18801, 18989,
	19179, 19370, 19564, 19760, 19957, 20157, 20358, 20562,
	20768, 20975, 21185, 21397, 21611, 21827, 22045, 22266,
	22488, 22713, 22940, 23170, 23401, 23635, 23872, 24111,
	24352, 24595, 24841, 25089, 25340, 25594, 25850, 26108,
	26369, 26633, 26899, 27168, 27440, 27714, 27992, 28271,
	28554, 28840, 29128, 29419, 29714, 30011, 30311, 30614,
	30920, 31229, 31542, 31857, 32176, 32497, 32822, 33151,
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

short mpu6500_big8_to_int16(const unsigned char *big8)
{
	short x;
	x = ((short)big8[0] << 8) | ((short)big8[1]);
	return x;
}

static int mpu6500_backup_register(struct i2c_client *client)
{
	int result = 0;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_PWR_MGMT_1,
				 2, mpu6500_selftest.pwm_mgmt);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_CONFIG,
				 1, &mpu6500_selftest.config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_GYRO_CONFIG,
				 1, &mpu6500_selftest.gyro_config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_USER_CTRL,
				 1, &mpu6500_selftest.user_ctrl);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_INT_ENABLE,
				 1, &mpu6500_selftest.int_enable);
	if (result)
		return result;

	result =
	    mpu6500_i2c_read_reg(client, MPUREG_SMPLRT_DIV,
				 1, &mpu6500_selftest.smplrt_div);
	if (result)
		return result;

	return result;
}

static int mpu6500_recover_register(struct i2c_client *client)
{
	int result = 0;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_1,
					 mpu6500_selftest.pwm_mgmt[0]);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_2,
					 mpu6500_selftest.pwm_mgmt[1]);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_CONFIG,
					 mpu6500_selftest.config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_GYRO_CONFIG,
					 mpu6500_selftest.gyro_config);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL,
					 mpu6500_selftest.user_ctrl);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_SMPLRT_DIV,
					 mpu6500_selftest.smplrt_div);
	if (result)
		return result;

	result =
	    mpu6500_i2c_write_single_reg(client, MPUREG_INT_ENABLE,
					 mpu6500_selftest.int_enable);
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

int mpu6500_selftest_run(struct i2c_client *client,
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
	result = mpu6500_backup_register(client);
	if (result) {
		printk(KERN_ERR "register backup error=%d", result);
		return result;
	}

	if (mpu6500_selftest.pwm_mgmt[0] & 0x40) {
		result = mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_1, 0x00);
		if (result) {
			printk(KERN_INFO "init PWR_MGMT error=%d", result);
			return result;
		}
	}

	regs[0] = mpu6500_selftest.pwm_mgmt[1] & ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	result = mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_2, regs[0]);

	result = mpu6500_i2c_write_single_reg(client, MPUREG_INT_ENABLE, 0);
	if (result)
		return result;

	/* disable the sensor output to FIFO */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, 0);
	if (result)
		return result;

	/* make sure the DMP is disabled first */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL, 0x00);
	if (result) {
		printk(KERN_INFO "DMP disable error=%d", result);
		return result;
	}

	/* clear FIFO */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL, BIT_FIFO_RST);
	if (result)
		return result;

	/* sample rate *//* = 1ms */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_SMPLRT_DIV, 0x00);
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
	result = mpu6500_i2c_write_single_reg(client, MPUREG_CONFIG, regs[0]);
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
	result = mpu6500_i2c_write_single_reg(client, MPUREG_GYRO_CONFIG, regs[0] << 3);
	if (result)
		return result;

	// Wait time
//	msleep(GYRO_WAIT_TIME);
	mdelay(200);

	// Enable FIFO
	result = mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL, BIT_FIFO_EN);
	if (result)
	  return result;

	// Enable gyro output to FIFO
	result = mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, BIT_GYRO_FIFO_EN);
	if (result)
	  return result;

	// Wait time
	mdelay(GYRO_WAIT_TIME);


	// Stop gyro FIFO
	result = mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, BIT_FIFO_DIS);
	if (result)
	  return result;

	// Read FIFO count
	result = mpu6500_i2c_read_reg(client, MPUREG_FIFO_COUNTH, 2, read_data);
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
	     result = mpu6500_i2c_read_reg(client, MPUREG_FIFO_R_W, FIFO_PACKET_SIZE, data);
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
	result = mpu6500_recover_register(client);
	if (result) {
		printk(KERN_ERR "register recovering error=%d", result);
		return result;
	}
	return ret_val;
}

static int mpu6500_do_powerup(struct i2c_client *client)
{
	int result = 0;
	char reg;

	mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_1, 0x1);

	mdelay(20);

	mpu6500_i2c_read_reg(client, MPUREG_PWR_MGMT_2, 1, &reg);

	reg &= ~(BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG);
	mpu6500_i2c_write_single_reg(client, MPUREG_PWR_MGMT_2, reg);

	return result;
}

static int mpu6500_do_test(struct i2c_client *client, int self_test_flag, int *gyro_result)
{
	int result, i, j, packet_size;
	u8 data[BYTES_PER_SENSOR * 2], d;
	int fifo_count, packet_count, ind, s;

	packet_size = BYTES_PER_SENSOR;

	result = mpu6500_i2c_write_single_reg(client, MPUREG_INT_ENABLE, 0);
	if (result)
		return result;
	/* disable the sensor output to FIFO */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, 0);
	if (result)
		return result;
	/* disable fifo reading */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL, 0);
	if (result)
		return result;
	/* clear FIFO */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_USER_CTRL, BIT_FIFO_RST);
	if (result)
		return result;
	/* setup parameters */
	result = mpu6500_i2c_write_single_reg(client, MPUREG_CONFIG, MPU_FILTER_184HZ);
	if (result)
		return result;
	result = mpu6500_i2c_write_single_reg(client, MPUREG_SMPLRT_DIV, 0x0);
	if (result)
		return result;
	result = mpu6500_i2c_write_single_reg(client, MPUREG_GYRO_CONFIG, self_test_flag | (MPU_FS_250DPS << 3));
	if (result)
		return result;

	/* wait for the output to get stable */
	mdelay(DEF_ST_STABLE_TIME);

	/* enable FIFO reading */
	result = mpu6500_i2c_write_single_reg(client,
			MPUREG_USER_CTRL, BIT_FIFO_EN);
	if (result)
		return result;
	/* enable sensor output to FIFO */
	d = BITS_GYRO_OUT;
	result = mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, d);
	if (result)
		return result;

	for (i = 0; i < THREE_AXIS; i++)
		gyro_result[i] = 0;

	s = 0;

	while (s < INIT_ST_SAMPLES) {
		mdelay(DEF_GYRO_WAIT_TIME);
		/* stop sending data to FIFO */
		result = mpu6500_i2c_write_single_reg(client, MPUREG_FIFO_EN, 0);
		if (result)
			return result;

		result = mpu6500_i2c_read_reg(client,
			MPUREG_FIFO_COUNTH, FIFO_COUNT_BYTE, data);

		if (result)
			return result;

		fifo_count = be16_to_cpup((__be16 *)(&data[0]));
		packet_count = fifo_count / packet_size;
		result = mpu6500_i2c_read_reg(client, MPUREG_FIFO_R_W,
			packet_size, data);

		if (result)
			return result;

		i = 0;

		while ((i < packet_count) && (s < INIT_ST_SAMPLES)) {
			result = mpu6500_i2c_read_reg(client, MPUREG_FIFO_R_W,
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

static int mpu6500_gyro_self_test(struct i2c_client *client,
		int *reg_avg, int *st_avg, int *ratio)
{
	int result;
	int ret_val;
	int ct_shift_prod[3], st_shift_cust[3], st_shift_ratio[3], i;
	u8 regs[3];

	ret_val = 0;
	result = mpu6500_i2c_read_reg(client, MPUREG_SELF_TEST_X_GYRO, 3, regs);
	if (result)
		return result;

	for (i = 0; i < 3; i++) {
		if (regs[i] != 0)
			ct_shift_prod[i] = gyro_6500_st_tb[regs[i] - 1];
		else
			ct_shift_prod[i] = 0;
	}

	pr_info("reg_bias : %d, %d, %d \n", reg_avg[0], reg_avg[1], reg_avg[2]);
	pr_info("st_avg : %d, %d, %d \n", st_avg[0], st_avg[1], st_avg[2]);

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

	pr_info("ct_shift_prod : %d %d %d\n", ct_shift_prod[0],
			ct_shift_prod[1], ct_shift_prod[2]);

	pr_info("st_shift_cust : %d %d %d\n", st_shift_cust[0],
			st_shift_cust[1], st_shift_cust[2]);

	pr_info("st_shift_ratio : %d %d %d\n", st_shift_ratio[0],
			st_shift_ratio[1], st_shift_ratio[2]);

	pr_err("%s, ret_val = %d\n", __func__, ret_val);

	return ret_val;
}


int mpu6500_gyro_hw_self_check(struct i2c_client *client, int ratio[3])
{
	int result;
	int gyro_bias_st[THREE_AXIS], gyro_bias_regular[THREE_AXIS];
	int test_times;
	int gyro_result;

	/*backup registers */
	result = mpu6500_backup_register(client);
	if (result) {
		pr_err("register backup error=%d", result);
		goto err_state;
	}

	/*power-up*/
	mpu6500_do_powerup(client);

	/*get regular bias*/
	test_times = DEF_ST_TRY_TIMES;
	while (test_times > 0) {
		result = mpu6500_do_test(client, 0, gyro_bias_regular);
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
		result = mpu6500_do_test(client,
			BITS_SELF_TEST_EN, gyro_bias_st);
		if (result == -EAGAIN)
			test_times--;
		else
			test_times = 0;
	}
	if (result)
		goto test_fail;

	gyro_result = mpu6500_gyro_self_test(client, gyro_bias_regular,
		gyro_bias_st, ratio);
	result = gyro_result;

err_state:
test_fail:
	/*recover registers */
	if (mpu6500_recover_register(client)) {
		pr_err("register recovering error\n");
		goto err_state;
	}

	pr_err("%s, gyro hw result = %d\n", __func__, result);

	return result;
}
