#ifndef _MPU6500_SELFTEST_H_
#define _MPU6500_SELFTEST_H_
int mpu6500_selftest_run(struct i2c_client *client,
			int packet_cnt[3],
			int gyro_bias[3],
			int gyro_rms[3],
			int gyro_lsb_bias[3]);
int mpu6500_gyro_hw_self_check(struct i2c_client *client, int ratio[3]);
#endif
