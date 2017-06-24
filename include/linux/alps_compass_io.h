/* alps_compass_io.h
 *
 * I/O controll header for alps sensor
 *
 * Copyright (C) 2011-2012 ALPS ELECTRIC CO., LTD. All Rights Reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef ___ALPS_IO_H_INCLUDED
#define ___ALPS_IO_H_INCLUDED


#include <linux/ioctl.h>

#define ALPSIO   0xAF

#define ALPSIO_SET_MAGACTIVATE   _IOW(ALPSIO, 0, int)
#define ALPSIO_SET_ACCACTIVATE   _IOW(ALPSIO, 1, int)
#define ALPSIO_SET_DELAY         _IOW(ALPSIO, 2, int)
#define ALPSIO_ACT_SELF_TEST_A   _IOR(ALPSIO, 3, int)
#define ALPSIO_ACT_SELF_TEST_B   _IOR(ALPSIO, 4, int)
#define ALPSIO_REOPT_VAL         _IOW(ALPSIO, 5, int)

//extern int accsns_get_acceleration_data(int *xyz);
extern int hscd_get_magnetic_field_data(int *xyz);
extern void hscd_activate(int flgatm, int flg, int dtime);
//extern void accsns_activate(int flgatm, int flg, int dtime);
extern int hscd_self_test_A(void);
extern int hscd_self_test_B(void);



#endif