/*
*
* drivers/media/isdbtmm/tuner_drv_sys.h
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

#ifndef _TUNER_DRV_SYS_H
#define _TUNER_DRV_SYS_H

#include <linux/ioctl.h>
/******************************************************************************
 * define
 ******************************************************************************/
/* IOCTL parameters ( I2C ) */
#define TUNER_IOC_MAGIC 'd'
#define TUNER_IOCTL_VALGET        	_IOW(TUNER_IOC_MAGIC, \
										 0x01, \
										 struct _tuner_data_rw)
#define TUNER_IOCTL_VALSET        	_IOR(TUNER_IOC_MAGIC, \
										 0x02, \
										 struct _tuner_data_rw)
#define TUNER_IOCTL_VALGET_EVENT  	_IOR(TUNER_IOC_MAGIC, \
										 0x03, \
										 struct _tuner_data_rw)
#define TUNER_IOCTL_VALSET_POWER  	_IOR(TUNER_IOC_MAGIC, \
										 0x04, \
										 struct _tuner_data_rw)
#define TUNER_IOCTL_VALSET_EVENT  	_IOW(TUNER_IOC_MAGIC, \
										 0x05, \
										 struct _tuner_data_rw)
#define TUNER_IOCTL_VALREL_EVENT  	_IOW(TUNER_IOC_MAGIC, \
										 0x06, \
										 struct _tuner_data_rw)
/* Add Start 20121218 No_1 */
#define TUNER_IOCTL_VALGET_OPENCNT	_IOW(TUNER_IOC_MAGIC, \
										 0x07, \
										 struct _tuner_sts_data)
#define TUNER_IOCTL_VALSET_MONICNT	_IOW(TUNER_IOC_MAGIC, \
										 0x08, \
										 struct _tuner_sts_data)
/* Add End 20121218 No_1 */

/* IOCTL parameters ( SPI ) */
#define TUNER_IOC_MAGIC_SPI 's'
#define TUNER_SPI_IOCTL_INIT		_IOR(TUNER_IOC_MAGIC_SPI, \
										 0x01, \
										 struct _ioctl_cmd)
#define TUNER_SPI_IOCTL_PKTSYNC 	_IOR(TUNER_IOC_MAGIC_SPI, \
										 0x02, \
										 struct _ioctl_cmd)
#define TUNER_SPI_IOCTL_BUFSTAT 	_IOW(TUNER_IOC_MAGIC_SPI, \
										 0x03, \
										 struct _ioctl_cmd)
#define TUNER_SPI_IOCTL_READPKT 	_IOW(TUNER_IOC_MAGIC_SPI, \
										 0x04, \
										 struct _ioctl_cmd)
#define	TUNER_SPI_IOCTL_PKTANSYNC 	_IOR(TUNER_IOC_MAGIC_SPI, \
										 0x05, \
										 struct _ioctl_cmd)

/*** SPI Sync Run or Stop Define ***/
#define SMT113J_SPI_SYNC_STOP			0
#define	SMT113J_SPI_SYNC_RUN			1

/* power supply parameters */
#define TUNER_DRV_CTL_POWON              0       /* Power ON  */
#define TUNER_DRV_CTL_POWOFF             1       /* Power OFF */

/* enabit enable */
#define TUNER_SET_ENADATA             0xFF       /* enabit    */

/******************************************************************************
 * data
 ******************************************************************************/
/* structure for register Read/Write I2C */
typedef struct _tuner_data_rw {
    unsigned short slave_adr;                     	/* I2C slave address  */
    unsigned short adr;                           	/* reg. address       */
    unsigned short sbit;                          	/* start bit position */
    unsigned short ebit;                          	/* end bit position   */
    unsigned short param;                         	/* write/read value   */
    unsigned short enabit;                        	/* enable bit mask    */
} TUNER_DATA_RW;

typedef struct _tuner_sts_data {
	unsigned long	open_cnt;
	unsigned long	moni_cnt;
} TUNER_STS_DATA;

/* for SPI ioctl data storage */
typedef struct _ioctl_cmd {
    unsigned int  val;                           	/* value */
    unsigned char *buf;                            	/* buffer */
} TUNER_DATA_SPI_RW;

#endif/* _TUNER_DRV_SYS_H */
