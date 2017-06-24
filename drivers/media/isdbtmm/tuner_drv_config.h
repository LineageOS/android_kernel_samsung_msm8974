/*
*
* drivers/media/isdbtmm/tuner_drv_config.h
*
* MM Tuner Driver
*
* Copyright (C) (2013, Samsung Electronics)
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation version 2.
*
* This program is distributed "as is" WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef _TUNER_DRV_CONFIG_H
#define _TUNER_DRV_CONFIG_H

/******************************************************************************
 * data
 ******************************************************************************/
#define TUNER_SET_ON                     1       /* setting ON               */
#define TUNER_SET_OFF                    0       /* setting OFF              */

/* device driver file name */
#define TUNER_CONFIG_DRIVER_NAME		"mmtuneri2c"

/* device number */
#define TUNER_CONFIG_DRV_MAJOR         225       /* MAJOR No.                */
#define TUNER_CONFIG_DRV_MINOR           0       /* MINOR No.                */

/* compile switch for IRQ */
/* #define TUNER_CONFIG_IRQ_PC_LINUX */

/* IRQ# */
#define TUNER_CONFIG_INT              0x07       /* IRQ No. ( Not Used )     */

/* I2C bus # */
#define TUNER_CONFIG_I2C_BUSNUM       0x0A       /* I2C Bus No.              */

/* kernel thread priority  */
#define TUNER_CONFIG_KTH_PRI            95       /* priority (0Å`99)         */

/* exclusive access control  */
#define TUNER_CONFIG_DRV_MULTI              	 /* exclusive control enable */

/* interrupt trigger type */
#define TUNER_CONFIG_IRQ_LEVEL  TUNER_SET_ON     /* trigger type = level     */

/* I2C slave address of tuner device register bank */
#define TUNER_SLAVE_ADR_S             0x60       /* reg. bank: Sub           */
#define TUNER_SLAVE_ADR_M1            0x61       /* reg. bank: Main#1        */
#define TUNER_SLAVE_ADR_M2            0x62       /* reg. bank: Main#2        */

#endif/* _TUNER_DRV_CONFIG_H */
