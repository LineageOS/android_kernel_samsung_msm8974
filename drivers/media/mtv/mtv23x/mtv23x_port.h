/******************************************************************************
* (c) COPYRIGHT 2013 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE		: MTV23x configuration header file.
*
* FILENAME	: mtv23x_port.h
*
* DESCRIPTION	:
*		Configuration for RAONTECH MTV23x Services.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE         NAME          REMARKS
* ----------  -------------    ------------------------------------------------
* 03/03/2013  Yang, Maverick       Created.
******************************************************************************/

#ifndef __MTV23X_PORT_H__
#define __MTV23X_PORT_H__

/*=============================================================================
* Includes the user header files if neccessry.
*===========================================================================*/
#if defined(__KERNEL__) /* Linux kernel */
	#include <linux/io.h>
	#include <linux/kernel.h>
	#include <linux/delay.h>
	#include <linux/mm.h>
	#include <linux/mutex.h>
	#include <linux/uaccess.h>
	#include <linux/string.h>
	#include <linux/jiffies.h>

#elif defined(WINCE) || defined(WINDOWS) || defined(WIN32)
	#include <stdio.h>
	#include <windows.h>
	#include <winbase.h>
	#include <string.h>
	#ifdef WINCE
		#include <drvmsg.h>
	#endif
#else
	#include <stdio.h>
	#include <string.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif

/*############################################################################
#
# COMMON configurations
#
############################################################################*/
/*============================================================================
* The slave address for I2C and SPI.
*===========================================================================*/
#define RTV_CHIP_ADDR	0x43  //7bit I2C Address ID

/*============================================================================
* Modifies the basic data types if necessary.
*===========================================================================*/
typedef int			BOOL;
typedef signed char		S8;
typedef unsigned char		U8;
typedef signed short		S16;
typedef unsigned short		U16;
typedef signed int		S32;
typedef unsigned int		U32;

typedef int			INT;
typedef unsigned int		UINT;
typedef long			LONG;
typedef unsigned long		ULONG;
 
typedef volatile U8		VU8;
typedef volatile U16		VU16;
typedef volatile U32		VU32;


#if defined(__GNUC__)
    #define INLINE   inline
#elif defined(WINCE) || defined(WINDOWS) || defined(WIN32)
    #define INLINE    __inline
#elif defined(__ARMCC_VERSION)
    #define INLINE    __inline
#else
    /* Need to modified */
    #define INLINE    inline
#endif

/*==============================================================================
* Selects the TV mode(s) to target product.
*============================================================================*/
#define RTV_ISDBT_ENABLE
//#define RTV_DVBT_ENABLE

#if defined(RTV_ISDBT_ENABLE)
//	#define RTV_ISDBT_1SEG_LPMODE_ENABLE
#endif 

/*============================================================================
* Defines the Dual Diversity Enable
*===========================================================================*/
//#define RTV_DUAL_DIVERISTY_ENABLE

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	extern U8 g_div_i2c_chip_id;
	#define RTV_CHIP_ADDR_SLAVE 0x44
	//	#defined RTV_DIVER_TWO_XTAL_USED //define for two X-TAL using both M,S 
#endif

/*============================================================================
* Defines the power Mode
*===========================================================================*/
//#define RTV_EXT_POWER_MODE

/*============================================================================
* Defines the package type of chip to target product.
*===========================================================================*/
//#define RTV_CHIP_PKG_CSP
#define RTV_CHIP_PKG_QFN

/*============================================================================
* Defines the external source frequency in KHz.
* Ex> #define RTV_SRC_CLK_FREQ_KHz	32000 // 32MHz
*===========================================================================*/
#define RTV_SRC_CLK_FREQ_KHz			32000//32MHz
//#define RTV_SRC_CLK_FREQ_KHz			19200//19.MHz
 
/*============================================================================
* Defines the Host interface.
*===========================================================================*/
#define RTV_IF_SPI /* AP: SPI Master Mode */
//#define RTV_IF_TSIF_0 /* I2C + TSIF0<GPDx pinout> for Serial Out Master Mode*/
//#define RTV_IF_TSIF_1 /* I2C + TSIF1<GDDx pinout> For Serial/Parallel Out Master Mode*/
//#define RTV_IF_SPI_SLAVE /* AP: SPI Slave Mode: control: I2C, data: SPI  */
//#define RTV_IF_SPI_TSIFx /* AP: SPI Master Mode for Control and please */
                           /*    define RTV_IF_TSIF_0  or RTV_IF_TSIF_1 for TSIF */

#if defined(RTV_IF_SPI_TSIFx)
#if defined(RTV_IF_TSIF_0)
	#error "TSIF0 can't use in case of SPI used for Register Control"
#endif
#endif

#if defined(RTV_IF_TSIF_0) || defined(RTV_IF_TSIF_1) || defined(RTV_IF_SPI_SLAVE)
//#define RTV_IF_CSI656_RAW_8BIT_ENABLE /*Sync signal pre-move(4clock) + 1 clock add Mode*/
#endif

#ifdef RTV_DUAL_DIVERISTY_ENABLE
	#if defined(RTV_IF_TSIF_1) || defined(RTV_IF_SPI) || defined(RTV_IF_SPI_SLAVE)
		 #error "Diversity function is not supported for defined interface"  
	#endif
#endif

/*============================================================================
* Defines the feature of SPI speed(> 30MHz) for SPI interface.
*===========================================================================*/
#if defined(RTV_IF_SPI)
	//#define RTV_SPI_HIGH_SPEED_ENABLE
#endif

/* Determine if the output of error-tsp is disable. */
#define RTV_ERROR_TSP_OUTPUT_DISABLE
#define RTV_NULL_PID_TSP_OUTPUT_DISABLE

#ifndef RTV_ERROR_TSP_OUTPUT_DISABLE
	/* Determine if the NULL PID will generated for error-tsp. */
	//#define RTV_NULL_PID_GENERATE
#endif /* RTV_ERROR_TSP_OUTPUT_DISABLE */

#if defined(RTV_IF_TSIF_0) && defined(RTV_IF_TSIF_1)
/*If TSIF0 & TSIF1 is Enabled, TS0= LayerA, TS1 = LayerB out..*/
//	#define DUAL_PORT_TSOUT_ENABLE
#endif

/*============================================================================
* Defines the polarity of interrupt if necessary.
*===========================================================================*/
#define RTV_INTR_POLARITY_LOW_ACTIVE
//#define RTV_INTR_POLARITY_HIGH_ACTIVE

/*============================================================================
* Defines the delay macro in milliseconds.
*===========================================================================*/
#if defined(__KERNEL__) /* Linux kernel */
	static INLINE void RTV_DELAY_MS(UINT ms)
	{
		unsigned long start_jiffies, end_jiffies;
		UINT diff_time;
		UINT _1ms_cnt = ms;

		start_jiffies = get_jiffies_64();

		do {
			end_jiffies = get_jiffies_64();
			
			diff_time = jiffies_to_msecs(end_jiffies - start_jiffies);
			if (diff_time >= ms)
				break;

			mdelay(1);		
		} while (--_1ms_cnt);
	}

#elif defined(WINCE)
	#define RTV_DELAY_MS(ms)    Sleep(ms)

#else
	void mtv_delay_ms(int ms);
	#define RTV_DELAY_MS(ms)    mtv_delay_ms(ms) /* TODO */
#endif

/*============================================================================
* Defines the debug message macro.
*===========================================================================*/
#if 1
    #define RTV_DBGMSG0(fmt)                   printk(fmt)
    #define RTV_DBGMSG1(fmt, arg1)             printk(fmt, arg1)
    #define RTV_DBGMSG2(fmt, arg1, arg2)       printk(fmt, arg1, arg2)
    #define RTV_DBGMSG3(fmt, arg1, arg2, arg3) printk(fmt, arg1, arg2, arg3)
#else
    /* To eliminates the debug messages. */
    #define RTV_DBGMSG0(fmt)			do {} while (0)
    #define RTV_DBGMSG1(fmt, arg1)		do {} while (0)
    #define RTV_DBGMSG2(fmt, arg1, arg2)	do {} while (0)
    #define RTV_DBGMSG3(fmt, arg1, arg2, arg3)	do {} while (0)
#endif
/*#### End of Common ###########*/

/*############################################################################
#
# ISDB-T specific configurations
#
############################################################################*/

/*############################################################################
#
# Host Interface specific configurations
#
############################################################################*/
#if defined(RTV_IF_SPI) || defined(RTV_IF_SPI_TSIFx)
    /*=================================================================
    * Defines the register I/O macros.
    *================================================================*/
	U8 mtv23x_spi_read(U8 page, U8 reg);
	void mtv23x_spi_read_burst(U8 page, U8 reg, U8 *buf, int size);
	void mtv23x_spi_write(U8 page, U8 reg, U8 val);
	extern U8 g_bRtvPage;

	static INLINE U8 RTV_REG_GET(U8 reg)
	{
		return (U8)mtv23x_spi_read(g_bRtvPage, (U8)(reg));
	}

	#define RTV_REG_BURST_GET(reg, buf, size)\
		mtv23x_spi_read_burst(g_bRtvPage, (U8)(reg), buf, (size))

	#define RTV_REG_SET(reg, val)\
		mtv23x_spi_write(g_bRtvPage, (U8)(reg), (U8)(val))

	#define RTV_REG_MASK_SET(reg, mask, val)\
	do {					\
		U8 tmp;				\
		tmp = (RTV_REG_GET(reg)|(U8)(mask))\
				& (U8)((~(mask))|(val));\
		RTV_REG_SET(reg, tmp);		\
	} while (0)

	#define RTV_TSP_XFER_SIZE	188
#endif

#if defined(RTV_IF_TSIF_0) || defined(RTV_IF_TSIF_1) ||  defined(RTV_IF_SPI_SLAVE)
	/*=================================================================
	* Defines the TS format.
	*================================================================*/
	//#define RTV_TSIF_FORMAT_0 /* Serial: EN_high, CLK_rising */
	#define RTV_TSIF_FORMAT_1 /*   Serial: EN_high, CLK_falling */ //
	//#define RTV_TSIF_FORMAT_2 /* Serial: EN_low, CLK_rising */
	//#define RTV_TSIF_FORMAT_3 /* Serial: EN_low, CLK_falling */
	//#define RTV_TSIF_FORMAT_4 /* Serial: EN_high, CLK_rising + 1CLK add */
	//#define RTV_TSIF_FORMAT_5 /* Serial: EN_high, CLK_falling + 1CLK add */
	//#define RTV_TSIF_FORMAT_6 /* Parallel: EN_high, CLK_rising */
	//#define RTV_TSIF_FORMAT_7 /* Parallel: EN_high, CLK_falling */

	/*=================================================================
	* Defines the TSIF speed.
	*================================================================*/
	//#define RTV_TSIF_SPEED_500_kbps  
	//#define RTV_TSIF_SPEED_1_Mbps  
	//#define RTV_TSIF_SPEED_2_Mbps 
	//#define RTV_TSIF_SPEED_4_Mbps
	//#define RTV_TSIF_SPEED_7_Mbps 
	//#define RTV_TSIF_SPEED_15_Mbps 
	#define RTV_TSIF_SPEED_30_Mbps 
	//#define RTV_TSIF_SPEED_60_Mbps 

	/*=================================================================
	* Defines the TSP size. 188 or 204
	*================================================================*/
	#define RTV_TSP_XFER_SIZE	188

	#ifndef RTV_IF_SPI_TSIFx
	/*=================================================================
	* Defines the register I/O macros.
	*================================================================*/
	U8 isdbt_i2c_read(U8 i2c_chip_addr, U8 reg);
	void isdbt_i2c_write(U8 i2c_chip_addr, U8 reg, U8 val);
	void isdbt_i2c_read_burst(U8 i2c_chip_addr, U8 reg, U8 *buf, int size);

	#ifdef RTV_DUAL_DIVERISTY_ENABLE		
		#define	RTV_REG_GET(reg)            		isdbt_i2c_read((U8)g_div_i2c_chip_id,(U8)reg)
		#define	RTV_REG_SET(reg, val)       		isdbt_i2c_write((U8)g_div_i2c_chip_id,(U8)reg, (U8)val)
		#define RTV_REG_BURST_GET(reg, buf, size)\
					isdbt_i2c_read_burst(g_div_i2c_chip_id, (U8)(reg), buf, (size))

	#else
		#define	RTV_REG_GET(reg)         isdbt_i2c_read(RTV_CHIP_ADDR, (U8)reg)
		#define	RTV_REG_SET(reg, val)      isdbt_i2c_write(RTV_CHIP_ADDR, (U8)reg, (U8)val)
		#define RTV_REG_BURST_GET(reg, buf, size)\
			isdbt_i2c_read_burst(RTV_CHIP_ADDR, (U8)(reg), buf, size)
	#endif

	#define	RTV_REG_MASK_SET(reg, mask, val) 								\
		do {																\
			U8 tmp;															\
			tmp = (RTV_REG_GET(reg)|(U8)(mask)) & (U8)((~(mask))|(val));	\
			RTV_REG_SET(reg, tmp);											\
		} while(0)
	#endif
#endif

#if defined(RTV_IF_EBI2)

	#define RTV_EBI2_BUS_WITDH_16 // 
	//#define RTV_EBI2_BUS_WITDH_32 //

	/*=================================================================
	* Defines the register I/O macros.
	*================================================================*/
	U8 isdbt_ebi2_read(U8 page, U8 reg);
	void isdbt_ebi2_read_burst(U8 page, U8 reg, U8 *buf, int size);
	void isdbt_ebi2_write(U8 page, U8 reg, U8 val);	
	extern U8 g_bRtvPage;

	static INLINE U8 RTV_REG_GET(U8 reg)
	{
		return (U8)isdbt_ebi2_read(g_bRtvPage, (U8)(reg));
	}

	#define RTV_REG_BURST_GET(reg, buf, size)\
		isdbt_ebi2_read_burst(g_bRtvPage, (U8)(reg), buf, (size))

	#define RTV_REG_SET(reg, val)\
		isdbt_ebi2_write(g_bRtvPage, (U8)(reg), (U8)(val))

	#define RTV_REG_MASK_SET(reg, mask, val)\
	do {					\
		U8 tmp;				\
		tmp = (RTV_REG_GET(reg)|(U8)(mask))\
				& (U8)((~(mask))|(val));\
		RTV_REG_SET(reg, tmp);		\
	} while (0)

	#define RTV_TSP_XFER_SIZE	188
#endif


/*############################################################################
#
# Pre-definintion by RAONTECH.
#
############################################################################*/


/*############################################################################
#
# Defines the critical object and macros.
#
############################################################################*/
#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
    #if defined(__KERNEL__)
	extern struct mutex raontv_guard;
	#define RTV_GUARD_INIT		mutex_init(&raontv_guard)
	#define RTV_GUARD_LOCK		mutex_lock(&raontv_guard)
	#define RTV_GUARD_FREE		mutex_unlock(&raontv_guard)
	#define RTV_GUARD_DEINIT	((void)0)

    #elif defined(WINCE) || defined(WINDOWS) || defined(WIN32)
	extern CRITICAL_SECTION		raontv_guard;
	#define RTV_GUARD_INIT		InitializeCriticalSection(&raontv_guard)
	#define RTV_GUARD_LOCK		EnterCriticalSection(&raontv_guard)
	#define RTV_GUARD_FREE		LeaveCriticalSection(&raontv_guard)
	#define RTV_GUARD_DEINIT	DeleteCriticalSection(&raontv_guard)
    #else
	/* temp: TODO */
	#define RTV_GUARD_INIT		((void)0)
	#define RTV_GUARD_LOCK		((void)0)
	#define RTV_GUARD_FREE		((void)0)
	#define RTV_GUARD_DEINIT	((void)0)
    #endif
#else
	#define RTV_GUARD_INIT		((void)0)
	#define RTV_GUARD_LOCK		((void)0)
	#define RTV_GUARD_FREE		((void)0)
	#define RTV_GUARD_DEINIT	((void)0)
#endif

/*############################################################################
#
# Check erros by user-configurations.
#
############################################################################*/
#if !defined(RTV_CHIP_PKG_CSP) && !defined(RTV_CHIP_PKG_QFN)
	#error "Must define the package type !"
#endif

#if defined(RTV_IF_TSIF_0) || defined(RTV_IF_TSIF_1) ||  defined(RTV_IF_SPI_SLAVE)\
|| defined(RTV_IF_SPI)
    #if (RTV_CHIP_ADDR >= 0xFF)
	#error "Invalid chip address"
    #endif

#elif defined(RTV_IF_EBI2)

#else
	#error "Must define the interface definition !"
#endif



#ifndef RTV_TSP_XFER_SIZE
	#error "Must define the RTV_TSP_XFER_SIZE definition !"
#endif

#if (RTV_TSP_XFER_SIZE != 188) && (RTV_TSP_XFER_SIZE != 204)
	#error "Must 188 or 204 for TS size"
#endif

#if defined(RTV_IF_SPI) || defined(RTV_IF_EBI2)
extern BOOL g_bRtvSpiHighSpeed;
#endif

void rtvOEM_PowerOn(int on);

#ifdef __cplusplus
}
#endif

#endif /* __MTV23X_PORT_H__ */

