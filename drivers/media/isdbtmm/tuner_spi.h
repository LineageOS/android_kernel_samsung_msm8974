/*
*
* drivers/media/isdbtmm/tuner_spi.h
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

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/fcntl.h>


/* for delay(sleep) */
#include <linux/delay.h>

/* for mutex */
#include <linux/mutex.h>

/*using copy to user */
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <asm/mach/irq.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <mach/gpio.h>

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
#define	NODE_MAJOR		226	 				/* Major No. */
#define	NODE_MINOR		0	 				/* Minor No. */
#define NODE_PATHNAME 	"mmtunerspi"  		/* Device Name */


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
/*** SPI Internal Register Setting ***/
typedef struct t_ioctl_spi_internal_reg
{
	unsigned char cmd;	/* 0x01 */
	unsigned char data;	/* 0xB1 */
} ioctl_spi_internal_reg;

/*** Packet Synchronize ***/
typedef struct t_ioctl_spi_pktsync
{
	unsigned char cmd; 	/* 0xD8 */
	unsigned char dum1;	/* 0x00 */
	unsigned char dum2; /* 0x00 */
	unsigned char dum3; /* 0x00 */
} ioctl_spi_pktsync;

/*** TS Buffer Status ***/
typedef struct t_ioctl_spi_bufstatus
{
	unsigned char cmd;		/* 0x03 */
	unsigned char read;		/* 0x10 */
	unsigned char addr;		/* 0x00 */
	unsigned char dum;		/* 0x00 */
} ioctl_spi_bufstatus;

/*** Read Packet ***/
typedef struct t_ioctl_spi_pktread
{
	unsigned char cmd; 		/* 0x0B */
	unsigned char dum1;		/* 0x00 */
	unsigned char size;		/* 0x01 */
	unsigned char read_pkt;	/* 0xXX */
	unsigned char dum2;		/* 0x00 */
} ioctl_spi_pktread;


/*** IOCTL Data ***/
typedef struct t_smt113j_ioctl_data
{
	unsigned int open_cnt;
}	smt113j_ioctl_data;

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
extern int 	smt113j_spi_thread 	( void  *arg );
extern int 	SMT113J_SPI_init	( void );
extern void SMT113J_SPI_exit	( void );


