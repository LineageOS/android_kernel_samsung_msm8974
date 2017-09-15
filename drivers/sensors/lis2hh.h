
/******************** (C) COPYRIGHT 2013 STMicroelectronics ********************
*
* File Name	: lis2hh_acc_sysfs.h
* Authors	: AMS - Motion Mems Division - Application Team
*		: Matteo Dameno (matteo.dameno@st.com)
* Version	: V.1.1.0
* Date		: 2013/Mar/28
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
*******************************************************************************/
/*******************************************************************************
Version History.

 Revision 1.0.0 25/Feb/2013
  first revision
  supports sysfs;
 Revision 1.1.0 28/Mar/2013
  introduces hr_timers for polling;
*******************************************************************************/

#ifndef	__LIS2HH_H__
#define	__LIS2HH_H__


#define	LIS2HH_ACC_DEV_NAME		"lis2hh_acc"

#define	LIS2HH_ACC_MIN_POLL_PERIOD_MS	2


#ifdef __KERNEL__

#define LIS2HH_ACC_SAD0L			(0x10)
#define LIS2HH_ACC_SAD0H			(0x01)
#define LIS2HH_ACC_I2C_SADROOT			(0x07)

/* I2C address if acc SA0 pin to GND */
#define LIS2HH_ACC_I2C_SAD_L		((LIS2HH_ACC_I2C_SADROOT<<2)| \
						LIS2HH_ACC_SAD0L)

/* I2C address if acc SA0 pin to Vdd */
#define LIS2HH_ACC_I2C_SAD_H		((LIS2HH_ACC_I2C_SADROOT<<2)| \
						LIS2HH_ACC_SAD0H)

/* to set gpios numb connected to interrupt pins,
 * the unused ones have to be set to -EINVAL
 */
#define LIS2HH_ACC_DEFAULT_INT1_GPIO		(-EINVAL)
#define LIS2HH_ACC_DEFAULT_INT2_GPIO		(-EINVAL)

/* Accelerometer Sensor Full Scale */
#define	LIS2HH_ACC_FS_MASK			(0x30)
#define LIS2HH_ACC_FS_2G			(0x00)
#define LIS2HH_ACC_FS_4G			(0x20)
#define LIS2HH_ACC_FS_8G			(0x30)


struct lis2hh_acc_platform_data {
	unsigned int poll_interval;
	unsigned int min_interval;

	u8 fs_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	u32 int_flags;
	/* set gpio_int[1,2] either to the choosen gpio pin number or to -EINVAL
	 * if leaved unconnected
	 */
	int gpio_int1;
	int gpio_int2;
};

#endif	/* __KERNEL__ */

#endif	/* __LIS2HH_H__ */



