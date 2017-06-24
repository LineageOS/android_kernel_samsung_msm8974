/*
*
* drivers/media/isdbtmm/tuner_spi.c
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
#include <linux/spi/spi.h>

#include <linux/wakelock.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

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

#include "tuner_spi.h"
#include "tuner.h"
#include "tuner_drv_sys.h"

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
DEFINE_MUTEX ( smt1113j_spi_device_lock );
#define MAX_BUFFER_PKT			0x1000						  /* 4MByte       */
#define MAX_RX_PACKET			100							  /* 100Packet    */
#define LOOP_RX_PAKET			20							  /* 20Packet     */
//#define MAX_LOOP_CNT			5							  /* spi loop cnt */
#define PAKCET_SIZE				188							  /* 1Packet size */
#define LOOP_RX_PACKET_BYTE		(LOOP_RX_PAKET * PAKCET_SIZE) /* 20 * 188Byte */

/* Debug Log */
/* #define USER_BUF_DEBUG */	 /* User Area Buffer Write Debug Log          */
/* #define KERNEL_BUF_DEBUG */	 /* Kernel Area Buffer Write Buffer Debug Log */

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static struct class 		*SMT113J_SPI_class = NULL;
static struct spi_device 	*smt113j_spi_device = NULL;
static struct wake_lock 	smt113j_spi_wake_lock;

/*** packet control info ***/
static unsigned char 	rx_pkt_buffer[(MAX_BUFFER_PKT * PAKCET_SIZE)] = { 0 };
static unsigned char 	rx_buf_temp[PAKCET_SIZE * 256] = { 0 };
static unsigned int 	pwrite = 0;
static unsigned int		pread  = 0;

static unsigned char spi_buf[(PAKCET_SIZE * LOOP_RX_PAKET)] __cacheline_aligned;

/*** open count ***/
static unsigned int 	spi_open_cnt = 0;

static unsigned int 	max_loop_count = 5;

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
/*** System Call ***/
static int 			 SMT113J_SPI_open		(struct inode *inode,
											 struct file *filp );
static int 			 SMT113J_SPI_release	(struct inode *inode, 
											 struct file *filp );
static ssize_t 		 SMT113J_SPI_read		(struct file *filp, 
											 char *buf, size_t count, 
											 loff_t * f_pos );
static int 			 SMT113J_SPI_write		(struct file *filp, 
											 const char __user * buf, 
											 size_t count, 
											 loff_t * f_pos );
static long 		 SMT113J_SPI_ioctl		(struct file *filp, 
											 unsigned int cmd, 
											 unsigned long arg );

/*** Platform Registory Call ***/
static int 			 SMT113J_SPI_remove		( struct platform_device *pdev );
static int 			 SMT113J_SPI_probe		( struct platform_device *pdev );

/*** SPI Driver Registory Call ***/
static int 			 smt113j_spi_probe		( struct spi_device *spi );
static int __devexit smt113j_spi_remove		( struct spi_device *spi );

/*** thread control ***/
static int 			 smt113j_spi_thread_Init 	( void );
static int 			 smt113j_spi_thread_Start 	( void );
static int 			 smt113j_spi_thread_Stop 	( void );
static int 			 smt113j_spi_thread_loop 	( void  *arg );
static int 			 smt113j_spi_thread_read 	( void );

/*** IOCTL ***/
static int 			 smt113j_spi_cmd_bufstat 	( unsigned char *status );
static int 			 smt113j_spi_cmd_pktsync 	( void );
static int 			 smt113j_spi_cmd_init 		( void );
static int 			 smt113j_spi_cmd_pktread 	( unsigned char *temp_buf );

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static const struct file_operations SMT113J_SPI_ctl_fops = {
	.owner          = THIS_MODULE,
	.open           = SMT113J_SPI_open,
	.write          = SMT113J_SPI_write,
	.read           = SMT113J_SPI_read,
	.unlocked_ioctl = SMT113J_SPI_ioctl,
	.release		= SMT113J_SPI_release,
};

static const struct of_device_id tmm_spi_data_match_table[] = {
	{   .compatible = "isdbtmm_spi_data",
	},
	{}
};

static struct platform_driver SMT113J_SPI_driver = {

	.probe	 = SMT113J_SPI_probe,
	.remove  = SMT113J_SPI_remove,
	.driver  = {
		.owner	= THIS_MODULE,
		.name = "tmmspi",
		.of_match_table = tmm_spi_data_match_table,
	},
};

static const struct of_device_id tmm_spi_match_table[] = {
	{   .compatible = "isdbtmm_spi_comp",
	},
	{}
};

static struct spi_driver smt113j_spi_driver = {
	.probe    = smt113j_spi_probe,
	.remove   = __devexit_p(smt113j_spi_remove),
	.driver = {
		.name = "tmmspidev",
		.owner = THIS_MODULE,
		.of_match_table = tmm_spi_match_table,
	},
};

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int smt113j_spi_cmd_init ( void )
{
	int	ret = 0;
	ioctl_spi_internal_reg spi_command = { 0 };
	
	struct spi_message  m    = {{ 0 }};
	struct spi_transfer t[3] = {{ 0 }};

	/*** SPI message init ***/
	spi_message_init ( &m );

	/*** SPI command set  ***/
	spi_command.cmd  = 0x01;
	spi_command.data = 0xB1;
	
	/*** transfer data set ***/
	t[0].tx_buf = &spi_command;
	t[0].rx_buf = NULL;
	t[0].len    = sizeof ( spi_command );
	
	spi_message_add_tail ( &t[0], &m );

	/*** SPI transfer request ***/
	ret = spi_sync ( smt113j_spi_device, &m );

	if ( 0 > ret ) 
	{
		ERROR_PRINT ("smt113j_spi_cmd_init : Sync Error << ret = %d >>", ret );
	}
	
	printk("smt113j_spi_cmd_init << End : %d >>\n", ret );
	
	return ( ret );
	
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int smt113j_spi_cmd_pktsync ( void )
{
	int ret = 0;
	ioctl_spi_pktsync spi_command = { 0 };
	
	struct spi_message  m    = {{ 0 }};
	struct spi_transfer t[3] = {{ 0 }};

	/*** SPI message init ***/
	spi_message_init ( &m );

	/*** SPI command set  ***/
	spi_command.cmd  = 0xD8;
	spi_command.dum1 = 0x00;
	spi_command.dum2 = 0x00;
	spi_command.dum3 = 0x00;
	
	/*** transfer data set ***/
	t[0].tx_buf = &spi_command;
	t[0].rx_buf = NULL;
	t[0].len    = sizeof ( spi_command );
	
	spi_message_add_tail ( &t[0], &m );

	/*** SPI transfer request ***/
	ret = spi_sync ( smt113j_spi_device, &m );

	if ( 0 > ret ) 
	{
		ERROR_PRINT ("smt113j_spi_cmd_pktsync : Sync Error << ret = %d >>", 
					 ret );
	}
	
	printk("smt113j_spi_cmd_pktsync << End : %d >>\n", ret );

	return ( ret );
}

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int smt113j_spi_cmd_bufstat ( unsigned char *status )
{

	int ret = 0;
	unsigned char data = 0;
	ioctl_spi_bufstatus spi_command = { 0 };

	struct 			spi_message  m    	= {{ 0 }};
	struct 			spi_transfer t[3] 	= {{ 0 }};

	DEBUG_PRINT("smt113j_spi_cmd_bufstat << Start : %x >>", 
				(unsigned int)*status );

	/*** argument check ***/
	if ( NULL == status )
	{
		ERROR_PRINT ("smt113j_spi_cmd_bufstat : Parameter Error");
		return ( -EINVAL );
	}
	
	/*** SPI message init ***/
	spi_message_init ( &m );

	/*** SPI command set  ***/
	spi_command.cmd  = 0x03;
	spi_command.read = 0x10;
	spi_command.addr = 0x00;
	spi_command.dum  = 0x00;
	
	/*** transfer data set ***/
	t[0].tx_buf = &spi_command;
	t[0].rx_buf = NULL;
	t[0].len    = sizeof ( spi_command );
	
	spi_message_add_tail ( &t[0], &m );

	/*** transfer data set ***/
	t[1].tx_buf = NULL;
	t[1].rx_buf = &data;
	t[1].len    = sizeof ( unsigned char );
	
	spi_message_add_tail ( &t[1], &m );

	/*** SPI transfer request ***/
	ret = spi_sync ( smt113j_spi_device, &m );

	if ( 0 > ret ) 
	{
		ERROR_PRINT ("smt113j_spi_cmd_bufstat : Sync Error << ret = %d >>", 
					 ret );
	}
	/*** read data set ***/
	*status = data;
	
	DEBUG_PRINT("smt113j_spi_cmd_bufstat << End : %x >>", *status );

	return ( ret );	

}

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int smt113j_spi_cmd_pktread ( unsigned char *temp_buf )
{
	int 				ret = 0;
	int 				loop = 0;
	static ioctl_spi_pktread 	spi_command __cacheline_aligned;

	struct spi_message  m    = {{ 0 }};
	struct spi_transfer t[3] = {{ 0 }};
	
	DEBUG_PRINT("smt113j_spi_cmd_pktread << Start >>");
	
	/*** argument check ***/
	if ( NULL == temp_buf )
	{
		ERROR_PRINT (
			"smt113j_spi_cmd_pktread : Paramerter Error  << parm = %x >>", 
			(unsigned int)temp_buf );
		return ( -EINVAL );
	}
	
	/*** SPI command set  ***/
	spi_command.cmd   	 = 0x0B;
	spi_command.dum1 	 = 0x00;
	/* TS Packet Size ( 0  : 188 Byte or 1: 204 Byte ) */
	spi_command.size 	 = 0x00;
	/* TS Packet Num  ( 20 : LOOP_RX_PAKET + 1 ) */
	spi_command.read_pkt = (LOOP_RX_PAKET - 1);
	spi_command.dum2	 = 0x00;
	
//	for ( loop = 0; loop < MAX_LOOP_CNT; loop++ )
	for ( loop = 0; loop < max_loop_count; loop++ )
	{
		/*** SPI message init ***/
		spi_message_init ( &m );
		
		memset ( t, 0, sizeof ( t ));
		memset ( spi_buf, 0, sizeof ( spi_buf ));
			
		/*** transfer data set ***/
		t[0].tx_buf = &spi_command;
		t[0].rx_buf = NULL;
		t[0].len    = sizeof ( spi_command );
		
		spi_message_add_tail ( &t[0], &m );

		/*** transfer data set ***/
		t[1].tx_buf = NULL;
		t[1].rx_buf = &spi_buf;
		t[1].len    = sizeof ( unsigned char ) * LOOP_RX_PACKET_BYTE;
		
		spi_message_add_tail ( &t[1], &m );

		/*** SPI transfer request ***/
		ret = spi_sync ( smt113j_spi_device, &m );

		if ( 0 > ret ) 
		{
			ERROR_PRINT ("smt113j_spi_cmd_pktread : Sync Error << ret = %d >>",
						 ret );
			break;
		}
		/*** read data copy ***/
		memcpy ( temp_buf, 
				 spi_buf, 
				 sizeof ( unsigned char ) * ( LOOP_RX_PACKET_BYTE ));
		
		temp_buf += LOOP_RX_PACKET_BYTE;
	}
	DEBUG_PRINT("smt113j_spi_cmd_pktread << End : %x >>", ret );
	
	return ( ret );	
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int SMT113J_SPI_open(struct inode *inode, struct file *filp)
{

	int ret = 0;
	unsigned int majorno = 0, 
				 minorno = 0;
	unsigned char *devarea = NULL;

	majorno = imajor(filp->f_dentry->d_inode);
	minorno = iminor(filp->f_dentry->d_inode);

	if (( majorno != NODE_MAJOR ) || 
		( minorno != NODE_MINOR )) 
	{
		ERROR_PRINT ("SMT113J_SPI_open Error : << No MAJOR(%d) or No MINOR(%d)",
					 majorno,
					 minorno );
		return ( -ENODEV );
	}
	
	/*** Private Data Area MALLOC Stataus Check ***/
	if ( NULL != ( devarea = (unsigned char *)filp->private_data ))
	{
		ERROR_PRINT(
			"SMT113J_SPI_open Error : << Used Private Data Area[0x%08x] >>", 
			(int)devarea );
		return ( -ENODEV );
	}
	
	/*** Private Data Area Alloc ***/
	if ( NULL == ( devarea = (unsigned char *)kmalloc ( 
								sizeof(smt113j_ioctl_data), GFP_KERNEL ))) 
	{
		ERROR_PRINT(
			"SMT113J_SPI_open Error : << Used Private Data Area Alloc Error");
		return ( -ENOMEM ) ;
	}
	memset ( devarea, 0,  sizeof ( smt113j_ioctl_data ));
	filp->private_data = (void *)devarea;
	
	/*** buffer clear ***/
	memset ( rx_pkt_buffer, 0, MAX_BUFFER_PKT * PAKCET_SIZE );
	pwrite = 0;
	pread  = 0;
	
	spi_open_cnt++;
	
	printk("SMT113J_SPI_open << End >>, spi_open_cnt = %d\n", spi_open_cnt);
	
	return ( ret );

}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int SMT113J_SPI_release(struct inode *inode, struct file *filp)
{
	int ret = 0;

	/*** Device Open Check ***/
	if ( 0 == spi_open_cnt )
	{
		ERROR_PRINT ("No Open Devices");
		return ( ret );
	}
	spi_open_cnt--;
	
	if ( 0 != spi_open_cnt )
	{
		ERROR_PRINT ("Not Relase : Open Device Num[%d]", spi_open_cnt );
		return ( ret );
	}
	
	/*** Private Data Area Check ***/
	if ( NULL == (unsigned char *)filp->private_data )
	{
		ERROR_PRINT("No Private Data");
		return ( ret );
	}
	/*** thread stop ***/
	smt113j_spi_thread_Stop();
	
	memset ( (unsigned char *)filp->private_data,
			 0,
			 sizeof ( smt113j_ioctl_data ));

	kfree ( filp->private_data );	
	
	filp->private_data = NULL;
	
	/*** buffer clear ***/
	memset ( rx_pkt_buffer, 0, MAX_BUFFER_PKT * PAKCET_SIZE );
	pwrite = 0;
	pread  = 0;
	
	printk("SMT113J_SPI_release << End >>, ret = %d\n", ret);

	return ret;
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static ssize_t SMT113J_SPI_read(struct file *filp, 
								char *buf, 
								size_t count, 
								loff_t * f_pos)
{
	int ret 	   = 0;
	int cnt		   = 0;
	int get_packet = 0;
	int tmp 	   = 0;
	int ring_flg   = 0;
	
#ifdef 	USER_BUF_DEBUG
	DEBUG_PRINT("SMT113J_SPI_read << Start >>");
#endif	/* USER_BUF_DEBUG */

	/*** Thread Status Check ***/
	if ( SMT113J_SPI_SYNC_RUN != spi_work_thread->status )
	{
		/*** no packet */
		printk("SMT113J_SPI_read << End : No Packet Sync Command(%d) >>\n", 
					spi_work_thread->status );
		return ( 0 );
	}

	if ( pwrite == pread )
	{
		/*** not received packet ***/
		printk("SMT113J_SPI_read << End : No Packet in kernel buffer>>\n");
		return ( 0 );
	}
	
	/*** calculation of the packet acquisition ***/
	if ( pwrite > pread )
	{
		/* not ring */
		get_packet = ( pwrite - pread ) / PAKCET_SIZE;
		ring_flg = 0;
	}
	else
	{
		/* ring */
		get_packet = ( pwrite + (MAX_BUFFER_PKT * PAKCET_SIZE) - pread ) 
					 / PAKCET_SIZE;
		ring_flg = 1;
	}
	
#ifdef 	USER_BUF_DEBUG
	DEBUG_PRINT("SMT113J_SPI_read << ring : %d >>", ring_flg );
	DEBUG_PRINT("-> pread = %d", pread );
	DEBUG_PRINT("-> pwrite = %d", pwrite );
	DEBUG_PRINT("-> get_packet = %d", get_packet );
	DEBUG_PRINT("-> Buffer Start Address = %x", &rx_pkt_buffer[0] );
	DEBUG_PRINT("-> Buffer End Address   = %x", 
				&rx_pkt_buffer[(MAX_BUFFER_PKT * PAKCET_SIZE) - 1] );
#endif	/* USER_BUF_DEBUG */
	
	if ( count > get_packet )
	{
		/* not ring */
		if ( 0 == ring_flg ) 
		{
			if ( copy_to_user ( buf, &rx_pkt_buffer[pread], 
								PAKCET_SIZE * get_packet ))
			{
				ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user1 Error << ret = %d >>", 
					ret );
				return ( -1 );
			}
			cnt = get_packet;
			pread += ( get_packet * PAKCET_SIZE );

#ifdef 	USER_BUF_DEBUG
			DEBUG_PRINT("-> Update RX Pcket = %d", cnt );
			DEBUG_PRINT("-> Update pread = %d", pread );
#endif	/* USER_BUF_DEBUG */
		}
		/* ring */
		else
		{
			if ( copy_to_user ( buf, &rx_pkt_buffer[pread], 
				 ( PAKCET_SIZE * ( MAX_BUFFER_PKT )) - pread ))
			{
				ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user2 Error << ret = %d >>", 
					ret );
				return ( -1 );
			}
#ifdef 	USER_BUF_DEBUG
			DEBUG_PRINT("-> buf = %x", buf );
			DEBUG_PRINT("-> update pread = %d", pread );
#endif	/* USER_BUF_DEBUG */

			cnt = (( PAKCET_SIZE * MAX_BUFFER_PKT ) - pread ) / PAKCET_SIZE;
			buf += ( PAKCET_SIZE * MAX_BUFFER_PKT ) - pread;

#ifdef 	USER_BUF_DEBUG
			DEBUG_PRINT("-> buf = %x", buf );
			DEBUG_PRINT("-> update RX Packet = %d", cnt );

			DEBUG_PRINT("-> Ring Buffer Control" );
#endif	/* USER_BUF_DEBUG */

			tmp = ( get_packet - cnt );
			if ( copy_to_user ( buf, &rx_pkt_buffer[0], PAKCET_SIZE * tmp))
			{
				ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user3 Error << ret = %d >>", 
					ret );
				return ( cnt );
			}
			cnt += tmp;
			pread = (PAKCET_SIZE * tmp);

#ifdef 	USER_BUF_DEBUG
			DEBUG_PRINT("-> pread = %d", pread );
			DEBUG_PRINT("-> update RX Packet = %d", cnt );
#endif	/* USER_BUF_DEBUG */

		}
	}
	else
	{
		/*** not ring ***/
		if ( 0 == ring_flg ) 
		{
			if ( copy_to_user ( buf, &rx_pkt_buffer[pread], 
								PAKCET_SIZE * count ))
			{
				ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user3 Error << ret = %d >>", 
					ret );
				return ( -1 );
			}
			cnt = count;
			pread += ( count * PAKCET_SIZE);

#ifdef 	USER_BUF_DEBUG
			DEBUG_PRINT("-> pread = %d", pread );
			DEBUG_PRINT("-> update RX Packet = %d", cnt );
#endif	/* USER_BUF_DEBUG */
		}
		/*** ring ***/
		else
		{
			cnt = 0;
	
			if ( count <= (MAX_BUFFER_PKT - (pread / PAKCET_SIZE)))
			{
				if ( copy_to_user ( buf, &rx_pkt_buffer[pread], 
								PAKCET_SIZE * count ))
				{
					ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user3 Error << ret = %d >>", 
					ret );
					return ( -1 );
				}
				cnt = count;
				pread += ( count * PAKCET_SIZE);
			}
			else
			{
			if ( copy_to_user ( buf, &rx_pkt_buffer[pread], 
				               ( PAKCET_SIZE * MAX_BUFFER_PKT ) - pread ))
			{
				ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user3 Error << ret = %d >>", 
					ret );
				return ( -1 );
			}

			cnt = MAX_BUFFER_PKT - ( pread / PAKCET_SIZE );
			buf += cnt * PAKCET_SIZE;

			tmp = ( count - cnt );
			if ( copy_to_user ( buf, &rx_pkt_buffer[0], PAKCET_SIZE * tmp ))
			{
				ERROR_PRINT(
					"SMT113J_SPI_read * copy_to_user3 Error << ret = %d >>", 
					ret );
					return ( -1 );
			}
			cnt += tmp;
			pread = ( PAKCET_SIZE * tmp);
			}
#ifdef 	USER_BUF_DEBUG
			DEBUG_PRINT("-> End pread = %d", pread );
			DEBUG_PRINT("-> End pwrite = %d", pwrite );
			DEBUG_PRINT("-> update RX Packet = %d", cnt );
#endif	/* USER_BUF_DEBUG */
		}
	}
#ifdef 	USER_BUF_DEBUG
	DEBUG_PRINT("SMT113J_SPI_read << End >>");
#else
	DEBUG_PRINT("SMT113J_SPI_read read packet = %d", cnt);
#endif
		
	return ( cnt );

}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int SMT113J_SPI_write(struct file *filp, 
							 const char __user * buf, 
							 size_t count, 
							 loff_t * f_pos)

{
	DEBUG_PRINT("SMT113J_SPI_write << Not Sppurt >>");
	
	return ( -ENOTSUPP );
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static long SMT113J_SPI_ioctl(struct file *filp, 
							  unsigned int cmd, 
							  unsigned long arg)
{
	int 				ret = 0;
	unsigned char 		pkt_status = 0;
	TUNER_DATA_SPI_RW 	*debug_buf = (TUNER_DATA_SPI_RW*)arg;
	
	DEBUG_PRINT("SMT113J_SPI_ioctl << Start >>");

	switch ( cmd )
	{
		/*** SPI INIT ***/
		case TUNER_SPI_IOCTL_INIT:
			printk ("-> TUNER_SPI_IOCTL_INIT\n");
			ret = smt113j_spi_cmd_init();
			break;

		/*** thread stop ***/
		case TUNER_SPI_IOCTL_PKTANSYNC:
			printk ("-> TUNER_SPI_IOCTL_PKTANSYNC\n");
			smt113j_spi_thread_Stop();
			break;
				
		/*** SPI Packet Synchronize ***/
		case TUNER_SPI_IOCTL_PKTSYNC:
			printk ("-> TUNER_SPI_IOCTL_PKTSYNC\n");
			if (debug_buf->buf == NULL) {
				printk ("debug_buf->buf is NULL. Setting max_loop_count to default\n");
				max_loop_count = 5;
			} else if (*debug_buf->buf == 1) {
				max_loop_count = 3;
			} else {
				max_loop_count = 5;
			}
			smt113j_spi_thread_Stop();
			ret = smt113j_spi_cmd_pktsync();
			smt113j_spi_thread_Start();
			break;

		/*** SPI Buffer Status ***/
		case TUNER_SPI_IOCTL_BUFSTAT:
			printk ("-> TUNER_SPI_IOCTL_BUFSTAT\n");
			ret = smt113j_spi_cmd_bufstat ( &pkt_status );
			
			if ( 0 != ret )
			{
				pkt_status = 0;
				ERROR_PRINT ("-> put_user(arg_data->moni_cnt) Error : ret = %d",
							 ret );
				break;
			}
			/* Copy to User Area */
			ret = put_user ( pkt_status, 
							(unsigned int __user *)&(debug_buf->val) );
			if ( 0 != ret )
			{
				ERROR_PRINT ("-> put_user(arg_data->moni_cnt) Error : ret = %d",
							 ret );
				break;
			}	
			DEBUG_PRINT ("-> pkt_status = %d", pkt_status );
			break;

		/*** SPI TS Packet Read ***/
		case TUNER_SPI_IOCTL_READPKT:
			ret = smt113j_spi_cmd_pktread ( rx_buf_temp );
		
			if ( 0 > ret )
			{
				ERROR_PRINT("TUNER_SPI_IOCTL_READPKT : Error << ret = %d >>", 
							ret );
				break;
			}
			ret = copy_to_user ( rx_buf_temp, 
								(unsigned char *)debug_buf->buf, 256 );
			if ( 0 > ret )
			{
				ERROR_PRINT(
				 "TUNER_SPI_IOCTL_READPKT * copy_to_user Error << ret = %d >>",
				 ret );
			}
			break;

		default:
			DEBUG_PRINT ("-> SMT113J_SPI_ioctl : No Coomand Case");
			ret = -EINVAL;
			break;
	}
	DEBUG_PRINT("SMT113J_SPI_ioctl << End >> : ret = %d", ret );

	return ( ret ) ;
}


/******************************************************************************
 *    function:   smt113j_spi_thread_Init
 *    brief   :   
 *    date    :   
 *    author  :   
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static int smt113j_spi_thread_Init ( void )
{
	int ret = 0;
	struct sched_param param = { .sched_priority = 99 };
	
	spi_work_thread = kmalloc ( sizeof ( smt113j_spi_thread_t ), GFP_KERNEL );
	
	if ( !spi_work_thread ) 
	{
		ERROR_PRINT ("smt113j_spi_thread_Init : Kmalloc Error");
		return ( -EFAULT );
	}

	spi_work_thread->status = SMT113J_SPI_SYNC_STOP;

	spin_lock_init( &spi_work_thread->tmm_lock );

	init_waitqueue_head ( &spi_work_thread->thread_wait );

	/*** Thread generation and run ***/
	spi_work_thread->thread_task = kthread_run ( smt113j_spi_thread_loop, 
												 NULL, 
												 "SMT113J_SPI_Task" );
	
	if ( IS_ERR ( spi_work_thread->thread_task )) 
	{
		ERROR_PRINT ("smt113j_spi_thread_Init : kthread_run error : %p", 
					 spi_work_thread->thread_task);
		goto ERROR2;
	}
	
	ret = sched_setscheduler ( spi_work_thread->thread_task, 
							   SCHED_FIFO, 
							   &param );
	
	if ( ret < 0 ) 
	{
		ERROR_PRINT (
			"smt113j_spi_thread_Init : sched_setscheduler error ret[ %d ]", 
			ret );
		goto ERROR3;
	}

	printk("smt113j_spi_thread_Init: End\n");

	return ( ret );

ERROR3:
	spi_work_thread->status = SMT113J_SPI_SYNC_STOP;
	wake_up_interruptible ( &( spi_work_thread->thread_wait ));
	kthread_stop ( spi_work_thread->thread_task );

ERROR2:
	kfree ( spi_work_thread );

	ERROR_PRINT ("smt113j_spi_thread_Init : Error");
	
	return ( ret );
}


/******************************************************************************
 *    function:   smt113j_spi_thread_Start
 *    brief   :   
 *    date    :   
 *    author  :   
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static int smt113j_spi_thread_Start ( void )
{
	printk("-> spi_work_thread->status[%d]\n", spi_work_thread->status );

	/* multiple check */
	if ( SMT113J_SPI_SYNC_RUN == spi_work_thread->status ) 
	{
		DEBUG_PRINT("smt113j_spi_thread_Start( Double ) : End!");
		return ( 0 );
	}
	/* thread run status set */
	spi_work_thread->status = SMT113J_SPI_SYNC_RUN;
	
	pwrite = 0;
	pread  = 0;
	
	wake_lock(&smt113j_spi_wake_lock);
	
	/* wakeup event */
	wake_up_interruptible ( &(spi_work_thread->thread_wait ));

	DEBUG_PRINT("smt113j_spi_thread_Start : End!");
	DEBUG_PRINT("-> spi_work_thread->status[%d]", spi_work_thread->status );

	return ( 0 );
}

/******************************************************************************
 *    function:   smt113j_spi_thread_Start
 *    brief   :   
 *    date    :   
 *    author  :   
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static int smt113j_spi_thread_Stop ( void )
{
	DEBUG_PRINT("smt113j_spi_thread_Stop : Start! << %s >>", 
				((spi_work_thread->status == SMT113J_SPI_SYNC_STOP)?
				"SMT113J_SPI_SYNC_STOP":
				(spi_work_thread->status == SMT113J_SPI_SYNC_RUN)?
				"SMT113J_SPI_SYNC_RUN":"other"));
	DEBUG_PRINT("-> spi_work_thread->status [%d]", spi_work_thread->status );

	mutex_lock(&smt1113j_spi_device_lock);

	spi_work_thread->status = SMT113J_SPI_SYNC_STOP;
	
	mutex_unlock(&smt1113j_spi_device_lock);

	wake_unlock(&smt113j_spi_wake_lock);
	
	DEBUG_PRINT("smt113j_spi_thread_Stop : End! << %s >>", 
				((spi_work_thread->status == SMT113J_SPI_SYNC_STOP)?
				"SMT113J_SPI_SYNC_STOP":
				(spi_work_thread->status == SMT113J_SPI_SYNC_RUN)?
				"SMT113J_SPI_SYNC_RUN":"other"));
	DEBUG_PRINT("-> spi_work_thread->status [%d]", spi_work_thread->status );
	
	return ( 0 );
}

/******************************************************************************
 *    function:   smt113j_spi_thread_loop
 *    brief   :   
 *    date    :   
 *    author  :   
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static int smt113j_spi_thread_loop ( void  *arg )
{
	int ret = 0;
	unsigned char status = 0;
	
	printk("smt113j_spi_thread_loop : Start!\n");

	while ( !kthread_should_stop() ) 
	{
		DEBUG_PRINT("smt113j_spi_thread_loop : spi_work_thread->status[%d]!", 
					spi_work_thread->status );

		/* wait event */
		if ( SMT113J_SPI_SYNC_RUN != spi_work_thread->status )
		{
			DEBUG_PRINT("smt113j_spi_thread_loop : Thread Wait!");
			
	 		ret = wait_event_interruptible ( spi_work_thread->thread_wait, 
	 					SMT113J_SPI_SYNC_STOP != spi_work_thread->status );

			if ( 0 != ret )
			{
				DEBUG_PRINT ("Not Interruptible : No Case");
				continue;
			}
			
			DEBUG_PRINT(
				"smt113j_spi_thread_loop : Thread Run << status[%d] >>!", 
				spi_work_thread->status );
		}
		/*** SPI Sync ***/
		switch ( spi_work_thread->status )
		{
			/*** SPI Sync Command ***/
			case SMT113J_SPI_SYNC_RUN:
				
				mutex_lock(&smt1113j_spi_device_lock);
				if ( spi_work_thread->status == SMT113J_SPI_SYNC_RUN )
				{
				/*** TS Buffering Status ***/
				ret = smt113j_spi_cmd_bufstat ( &status );
				}
				mutex_unlock(&smt1113j_spi_device_lock);
				
				if ( 0 > ret )
				{
					ERROR_PRINT ("smt113j_spi_thread_loop : Pakcet Error = %d",
								 ret );
					continue;
				}
				DEBUG_PRINT("smt113j_spi_thread_loop : data ready buf = 0x%x", status);	
				/*** Pakcet state ( status = 0x00<not ready>/0x01<ready> ) ***/
				if ( 0x01 == ( status & 0x01 ))
				{
					DEBUG_PRINT (
						"smt113j_spi_thread_read() before : status[%d]", 
						spi_work_thread->status );

					mutex_lock(&smt1113j_spi_device_lock);
					if ( spi_work_thread->status == SMT113J_SPI_SYNC_RUN )
					{
					/*** Buffer Ready ***/
					smt113j_spi_thread_read();
					}
					mutex_unlock(&smt1113j_spi_device_lock);

					DEBUG_PRINT (
						"smt113j_spi_thread_read() after : status[%d]", 
						spi_work_thread->status );

				}
				else
				{
					printk("smt113j_spi_thread_loop : No data in tuner buffer or Error occured\n");
					
					/*** inside chip buffer is over or under run check ***/
					if ( 0 != ( status & 0x06 ))
					{
						/*** Over or Under Run ***/
						ERROR_PRINT (
							"smt113j_spi_thread_loop : buffer Error < %s >",
							((0x02 == (status & 0x02))?
							"Under-run":((0x04 == (status & 0x04))?
							"Over-run":"No Error")));
						smt113j_spi_cmd_pktsync();
					}
					else
					{
						DEBUG_PRINT (
							"smt113j_spi_thread_loop : Buffering Wait");
					}
					/* 20ms sleep */
					usleep_range (( 20 * 1000 ), ( 20 * 1000 ));
				}
				break;
			default:
				/*** Thread Status Error ***/
				ERROR_PRINT (
					"smt113j_spi_thread_loop : Thread Status Error : %d",
					spi_work_thread->status );
					spi_work_thread->status = SMT113J_SPI_SYNC_STOP;
				break;
		}
	}
	DEBUG_PRINT("smt113j_spi_thread_loop : End!");

	return ( 0 );
}


/******************************************************************************
 *    function:   smt113j_spi_thread_read
 *    brief   :   
 *    date    :   
 *    author  :   
 *
 *    return  :   none
 *    input   :   none
 *    output  :   none
 ******************************************************************************/
static int smt113j_spi_thread_read ( void )
{
	int ret = 0;

#ifdef 	KERNEL_BUF_DEBUG
	int cnt = 0;
	int temp_write = pwrite;
#endif	/* KERNEL_BUF_DEBUG */
	
	DEBUG_PRINT("smt113j_spi_thread_read : Start!");

	/*** Read Packet***/
	ret = smt113j_spi_cmd_pktread ( rx_buf_temp );
	
#ifdef 	KERNEL_BUF_DEBUG
	DEBUG_PRINT("pwrite = %d", pwrite );
	DEBUG_PRINT("pread = %d", pread );
	
	/*** debug test ***/
	for ( cnt = 0; cnt < 20; cnt++ )
	{
		DEBUG_PRINT("read[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]!", 
					rx_buf_temp[pread + (cnt*188)+0],
					rx_buf_temp[pread + (cnt*188)+1],
					rx_buf_temp[pread + (cnt*188)+2],
					rx_buf_temp[pread + (cnt*188)+3],
					rx_buf_temp[pread + (cnt*188)+4] );
	}
#endif	/* KERNEL_BUF_DEBUG */

	if ( 0 > ret )
	{
		ERROR_PRINT ("smt113j_spi_thread_read : Pakcet Error = %d", ret );
		return ( ret );
	}

	if ((( pwrite / PAKCET_SIZE ) + MAX_RX_PACKET ) > MAX_BUFFER_PKT )
	{
		memcpy ( &rx_pkt_buffer[pwrite], 
				 rx_buf_temp, 
				 (PAKCET_SIZE * MAX_BUFFER_PKT) - pwrite );
#ifdef 	KERNEL_BUF_DEBUG
		for ( cnt = 0; cnt < 20; cnt++ )
		{
			DEBUG_PRINT(
				"read[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]!", 
				rx_pkt_buffer[pwrite+(cnt*188)+0],
				rx_pkt_buffer[pwrite+(cnt*188)+1],
				rx_pkt_buffer[pwrite+(cnt*188)+2],
				rx_pkt_buffer[pwrite+(cnt*188)+3],
				rx_pkt_buffer[pwrite+(cnt*188)+4] );
		}
#endif	/* KERNEL_BUF_DEBUG */
		memcpy ( rx_pkt_buffer, 
				 &rx_buf_temp[(PAKCET_SIZE * MAX_BUFFER_PKT) - pwrite], 
				 ( PAKCET_SIZE * MAX_RX_PACKET ) - 
				 (( PAKCET_SIZE * MAX_BUFFER_PKT ) - pwrite ));

#ifdef 	KERNEL_BUF_DEBUG
		for ( cnt = 0; cnt < 20; cnt++ )
		{
			DEBUG_PRINT(
				"read[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]!",
				rx_pkt_buffer[(cnt*188)+0],
				rx_pkt_buffer[(cnt*188)+1],
				rx_pkt_buffer[(cnt*188)+2],
				rx_pkt_buffer[(cnt*188)+3],
				rx_pkt_buffer[(cnt*188)+4] );
		}
#endif	/* KERNEL_BUF_DEBUG */

		pwrite = ((PAKCET_SIZE * MAX_RX_PACKET) - 
				 ((PAKCET_SIZE * MAX_BUFFER_PKT) - pwrite ));
	}
	else
	{
		memcpy ( &rx_pkt_buffer[pwrite],
				 rx_buf_temp,
				 PAKCET_SIZE * MAX_RX_PACKET );

		pwrite += (PAKCET_SIZE * MAX_RX_PACKET);

#ifdef 	KERNEL_BUF_DEBUG
		for ( cnt = 0; cnt < 20; cnt++ )
		{
			DEBUG_PRINT(
				"read[0x%02x][0x%02x][0x%02x][0x%02x][0x%02x]!",
				rx_pkt_buffer[temp_write+(cnt*188)+0],
				rx_pkt_buffer[temp_write+(cnt*188)+1],
				rx_pkt_buffer[temp_write+(cnt*188)+2],
				rx_pkt_buffer[temp_write+(cnt*188)+3],
				rx_pkt_buffer[temp_write+(cnt*188)+4] );
		}
#endif	/* KERNEL_BUF_DEBUG */
	}
	DEBUG_PRINT("smt113j_spi_thread_read : End!");
	
	return ( ret );
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int smt113j_spi_probe(struct spi_device *spi)
{
	int ret;
	spi->bits_per_word = 8;
	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = (27 * 1000 * 1000); /* 30MHz */

	ret = spi_setup ( spi );
	if ( ret < 0 ) 
	{
		ERROR_PRINT("smt113j_spi_probe : spi_setup Failed = %d!", ret );
		return ( ret );
	}
	smt113j_spi_device = spi;

	wake_lock_init ( &smt113j_spi_wake_lock, 
					 WAKE_LOCK_SUSPEND, 
					 "smt113j_spi_wake_lock" );
	
	printk("smt113j_spi_probe : End!\n");

	return ( 0 );
}

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int __devexit smt113j_spi_remove(struct spi_device *spi)
{
	smt113j_spi_device = NULL;

	printk ("smt113j_spi_remove : End\n");
	
	return 0;
}

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int SMT113J_SPI_probe(struct platform_device *pdev)

{
	int 	ret;
	struct	device *SMT113J_SPI_dev;

 	ret = register_chrdev ( NODE_MAJOR, NODE_PATHNAME, &SMT113J_SPI_ctl_fops );
	
	if ( 0 > ret ) 
	{
		ERROR_PRINT("SMT113J_SPI_probe : register_chrdev failed : ret = %d!", 
					ret );
		return ( -EFAULT );
	}
	
	SMT113J_SPI_class = class_create(THIS_MODULE, NODE_PATHNAME);

	if (IS_ERR(SMT113J_SPI_class)) 
	{
		ERROR_PRINT("SMT113J_SPI_probe : class_create failed : ret = %d!", 
					ret );
		unregister_chrdev(NODE_MAJOR, NODE_PATHNAME);
		class_destroy(SMT113J_SPI_class);
		return ( -EFAULT );
	}

	SMT113J_SPI_dev = device_create ( SMT113J_SPI_class, 
									  NULL,
									  MKDEV(NODE_MAJOR, NODE_MINOR),
									  NULL, 
									  NODE_PATHNAME);

				

	if (IS_ERR(SMT113J_SPI_dev)) 
	{
		ERROR_PRINT("SMT113J_SPI_probe : device_create failed : ret = %d!", 
					ret );
		unregister_chrdev(NODE_MAJOR, NODE_PATHNAME);
		class_destroy(SMT113J_SPI_class);
		return ( -EFAULT );
	}
	ret = spi_register_driver ( &smt113j_spi_driver );
	
	if (ret < 0) 
	{
		ERROR_PRINT(
			"SMT113J_SPI_probe : spi_register_driver failed : ret = %d!",
			ret );
		return ( -EBUSY );
	}

	smt113j_spi_thread_Init();
	
	printk("SMT113J_SPI_probe : End!\n");
	
	return 0;
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
static int SMT113J_SPI_remove ( struct platform_device *pdev )
{
	int ret = 0;
	
	spi_unregister_driver ( &smt113j_spi_driver );

	printk("SMT113J SMT113J_SPI_remove : End\n");
	
	return ( ret );
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
int SMT113J_SPI_init(void)
{
	int ret = 0;

	DEBUG_PRINT("SMT113J_SPI_init : Start");

	ret = platform_driver_register ( &SMT113J_SPI_driver );
	if ( ret )
	{
		ERROR_PRINT("SMT113J_SPI_init : Error = %x", ret);
		
		return ( ret );
	}
	DEBUG_PRINT("SMT113J_SPI_init : End = %x", ret);

	return ( 0 );
}


/******************************************************************************/
/*** 																		***/
/******************************************************************************/
void SMT113J_SPI_exit(void)
{
	DEBUG_PRINT("SMT113J SMT113J_SPI_exit : Start");

	unregister_chrdev ( NODE_MAJOR, NODE_PATHNAME );

	device_destroy ( SMT113J_SPI_class, MKDEV(NODE_MAJOR, NODE_MINOR ));

	class_destroy ( SMT113J_SPI_class );

	platform_driver_unregister ( &SMT113J_SPI_driver );

	DEBUG_PRINT("SMT113J SMT113J_SPI_exit : End");
	
	return;
}

/******************************************************************************/
/*** 																		***/
/******************************************************************************/
MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("ISDBTMM Driver");
MODULE_LICENSE("GPL v2");

