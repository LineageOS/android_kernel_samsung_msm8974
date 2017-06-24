/*
*
* drivers/media/isdbtmm/tuner.c
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

/******************************************************************************
 * include
 ******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <linux/wakelock.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

#include "tuner_drv.h"
#include "tuner_spi.h"
#include "tuner.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
#include <linux/mutex.h>
#endif

/******************************************************************************
 * data
 ******************************************************************************/
/*** thread control ***/
smt113j_spi_thread_t *spi_work_thread = NULL;

/******************************************************************************
 * function
 ******************************************************************************/

extern void driver_config_during_boot_up(void);

/******************************************************************************
 * code area
 ******************************************************************************/
/******************************************************************************
 *    function:   smt113J_tuner_start
 *    brief   :   initialization control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static int __init smt113J_tuner_start (void)
{
	/* I2C Driver initialization module */
	tuner_drv_start();

	/* SPI Driver initialization module */
	SMT113J_SPI_init();
	
    printk("smt113J_tuner_start: End\n");

	driver_config_during_boot_up();

    return 0;
}

/******************************************************************************
 *    function:   smt113J_tuner_end
 *    brief   :   exit control of a driver
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static void __exit smt113J_tuner_end ( void )
{
	/* I2C Driver exit module */
	tuner_drv_end();

	/* SPI Driver exit module */
	SMT113J_SPI_exit();
	
    printk("smt113J_tuner_end: End\n");

	return;
}

/******************************************************************************/
/***																		***/
/******************************************************************************/
module_init ( smt113J_tuner_start );
module_exit ( smt113J_tuner_end );


/******************************************************************************/
/***																		***/
/******************************************************************************/
MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("MM Tuner Driver");
MODULE_LICENSE("GPL v2");

