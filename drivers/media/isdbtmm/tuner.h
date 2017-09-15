/*
*
* drivers/media/isdbtmm/tuner.h
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
/***																		***/
/******************************************************************************/
#ifndef _TUNER_H_
#define _TUNER_H_

#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kthread.h>

/******************************************************************************
 * Typedef
 ******************************************************************************/
typedef struct smt113j_SPI_thread 
{
	struct task_struct  *thread_task;
	wait_queue_head_t 	thread_wait;
	u8 					status;
	spinlock_t 			tmm_lock;
}	smt113j_spi_thread_t;

/******************************************************************************
 * extern
 ******************************************************************************/
extern smt113j_spi_thread_t *spi_work_thread;


/******************************************************************************
 * extern
 ******************************************************************************/
/* DEBUG */
/*#define DLOG*/
#ifdef  DLOG
#define INFO_PRINT( fmt, arg... )   { printk(KERN_INFO "%s: " fmt "\n", "mmtuner driver", ##arg); }
#define DEBUG_PRINT( fmt, arg... )  { printk(KERN_INFO "%s: " fmt "\n", "mmtuner driver", ##arg); }
#define TRACE() DEBUG_PRINT( "%s( %d )", __FUNCTION__, __LINE__ )
#else
#define INFO_PRINT( fmt, arg... )  
#define DEBUG_PRINT( fmt, arg... )
#define TRACE() DEBUG_PRINT()
#endif /* DLOG */
#define ERROR_PRINT( fmt, arg... )  { printk(KERN_ERR  "%s: " fmt "\n", "mmtuner driver", ##arg); }

/******************************************************************************
 * extern
 ******************************************************************************/
extern int smt113J_tuner_SpiThreadStart ( void );


#endif /* _TUNER_DRV_H */

