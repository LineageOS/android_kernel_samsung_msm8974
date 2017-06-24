/*
*
* drivers/media/isdbtmm/tuner_drv_hw.c
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
/* include                                                                    */
/******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <asm/uaccess.h>
#include "tuner_drv.h"

#include <linux/mm.h>
#include <linux/vmalloc.h>


/* #define USER_DEBUG_LOG */

/******************************************************************************
 * function
 ******************************************************************************/
int tuner_drv_hw_access( unsigned int uCommand, TUNER_DATA_RW *data,
                         unsigned short param_len );

/******************************************************************************
 * code area
 ******************************************************************************/
/******************************************************************************
 *    function:   tuner_drv_hw_access
 *    brief   :   HW access control of a driver
 *                repeat specified count single register access
 *    date    :   2011.08.02
 *    author  :   K.Kitamura(*)
 *    modifier:   K.Okawa(KXD14)
 *
 *    return  :    0                   normal exit
 *            :   -1                   error exit
 *    input   :   uCommand             setting command
 *            :   data                 access data
 *            :   param_len            nummber of access data
 *    output  :   none
 ******************************************************************************/
int tuner_drv_hw_access( unsigned int uCommand, TUNER_DATA_RW *data,
                         unsigned short param_len )
{
    int                ret;
    struct i2c_adapter *adap = NULL;
    struct i2c_msg     msgs[2];
    unsigned short     addr;
    unsigned short     flags;
    unsigned char      buf[TUNER_I2C_MSG_DATA_NUM];
    unsigned char      read_data;
    unsigned char      ena_data;
    unsigned char      write_data;
    unsigned short     loop_cnt;


    /* argument check */
    if( data == NULL )
    {
        TRACE();
        return -EINVAL;
    }

    /* get i2c adapter */
    adap = i2c_get_adapter( TUNER_CONFIG_I2C_BUSNUM );
    if( adap == NULL )
    {
        TRACE();
        return -EINVAL;
    }

    /* initialize */
    memset( msgs, 0x00, sizeof(struct i2c_msg) * 2 );
    ena_data = 0x00;
    flags = 0;
    /* access loop */
    for( loop_cnt = 0; loop_cnt < param_len; loop_cnt++ ) {
        /* initialize i2c messages */
        memset( msgs, 0x00, sizeof(struct i2c_msg) * 2 );
        ena_data = 0x00;

        /* command detect switch */
        switch ( uCommand ) {
             case TUNER_IOCTL_VALGET:	/* read access */
                addr   = data[ loop_cnt ].slave_adr;
                flags  = I2C_M_RD;
                buf[ 0 ] = (unsigned char)data[ loop_cnt ].adr;
                buf[ 1 ] = 0;
                break;
            case TUNER_IOCTL_VALSET:	/* write access */
                addr   = data[ loop_cnt ].slave_adr;
                flags  = 0;
                buf[ 0 ] = (unsigned char)data[ loop_cnt ].adr;
                buf[ 1 ] = (unsigned char)data[ loop_cnt ].param;
                break;
            default:
                i2c_put_adapter( adap );
                return -EINVAL;
        }

        if( flags != I2C_M_RD ) {
        	 /* bit modify write */
        	if( !(( data[ loop_cnt ].sbit == 0 ) && 
        	 	  ( data[ loop_cnt ].ebit == 7 )))
        	{
            	/* specified start/end bit position mode */
                /* TODO: enabit */
                if(( data[ loop_cnt ].sbit == TUNER_SET_ENADATA )
                && ( data[ loop_cnt ].ebit == TUNER_SET_ENADATA ))
                {
                    ena_data = ( unsigned char )data[ loop_cnt ].enabit; 
                }
                else
                {
                    /* calculate enable bit mask */
                	ena_data = (unsigned char)(((1U << 
                			(1+data[loop_cnt].ebit-data[loop_cnt].sbit))-1) <<
                			data[loop_cnt].sbit);
                }
                
                if( ena_data != 0xFF )
                {
                    /* read a current value */
                    msgs[ 0 ].addr  = addr;
                    msgs[ 0 ].flags = 0;
                    msgs[ 0 ].len   = TUNER_R_MSGLEN;
                    msgs[ 0 ].buf   = &buf[ 0 ];
                    msgs[ 1 ].addr  = addr;
                    msgs[ 1 ].flags = I2C_M_RD;
                    msgs[ 1 ].len   = TUNER_R_MSGLEN;
                    msgs[ 1 ].buf   = &read_data;

                    ret = i2c_transfer( adap, msgs, TUNER_R_MSGNUM );
                    if( ret < 0 )
                    {
                        TRACE();
                        i2c_put_adapter( adap );
                        return -EINVAL;
                    }

                    /* initialize i2c message */
                    memset( msgs, 0x00, sizeof( struct i2c_msg ) * 2 );

                    /* clear bits of write position */
                    read_data  &= ( unsigned char )( ~ena_data );
                    /* construct a write value */
                    write_data = ( unsigned char )( ena_data & 
                    								data[ loop_cnt ].param );
                    buf[ 1 ] = ( unsigned char )( write_data | read_data );
                }
            }

#ifdef USER_DEBUG_LOG
            DEBUG_PRINT(
            "ioctl(W) slv:0x%02x adr:0x%02x 0x%02x (RMW:0x%02x WDAT:0x%02x)",
            addr, buf[ 0 ], data[ loop_cnt ].param, read_data,  buf[ 1 ]);
#endif /* USER_DEBUG_LOG */

            msgs[ 0 ].addr  = addr;
            msgs[ 0 ].flags = flags;
            msgs[ 0 ].len   = TUNER_W_MSGLEN;
            msgs[ 0 ].buf   = buf;


            ret = i2c_transfer( adap, msgs, TUNER_W_MSGNUM );
            if( ret < 0 )
            {
                TRACE();
                i2c_put_adapter( adap );
                return -EINVAL;
            }
        } else {
        	/* write register */
            msgs[ 0 ].addr  = addr;
            msgs[ 0 ].flags = 0;
            msgs[ 0 ].len   = TUNER_R_MSGLEN;
            msgs[ 0 ].buf   = &buf[ 0 ];
            msgs[ 1 ].addr  = addr;
            msgs[ 1 ].flags = flags;
            msgs[ 1 ].len   = TUNER_R_MSGLEN;
            msgs[ 1 ].buf   = &buf[ 1 ];

#ifdef USER_DEBUG_LOG
            DEBUG_PRINT("ioctl(read(Pre)) slv:0x%02x adr:0x%02x (single)",
								addr, buf[ 0 ]);
#endif /* USER_DEBUG_LOG */
            ret = i2c_transfer( adap, msgs, TUNER_R_MSGNUM );
            if( ret < 0 )
            {
                TRACE();
                i2c_put_adapter( adap );
                return -EINVAL;
            }

            /* return read val. */
            data[ loop_cnt ].param = buf[ 1 ];
#ifdef USER_DEBUG_LOG
            DEBUG_PRINT(
            	"ioctl(R) slv:0x%02x adr:0x%02x 0x%02x (RETURN:0x%02x)",
                addr, buf[ 0 ], buf[1], data[ loop_cnt ].param);
#endif /* USER_DEBUG_LOG */

        }
    }

    i2c_put_adapter( adap );
    return 0;
}
