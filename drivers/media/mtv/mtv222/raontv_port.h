/******************************************************************************
* (c) COPYRIGHT 2010 RAONTECH, Inc. ALL RIGHTS RESERVED.
*
* TITLE 	  : RAONTECH TV configuration header file. 
*
* FILENAME    : raontv_port.h
*
* DESCRIPTION :
*		Configuration for RAONTECH TV Services.
*
******************************************************************************/
/******************************************************************************
* REVISION HISTORY
*
*    DATE	  	  NAME				REMARKS
* ----------  -------------    ------------------------------------------------
* 07/26/2013  Yang, Maverick   Created.
******************************************************************************/

#ifndef __RAONTV_PORT_H__
#define __RAONTV_PORT_H__

/*=============================================================================
 * Includes the user header files if neccessry.
 *============================================================================*/ 
#if defined(__KERNEL__) /* Linux kernel */
    #include <linux/io.h>
    #include <linux/kernel.h>
    #include <linux/delay.h>
    #include <linux/mm.h>
    #include <linux/mutex.h>
    #include <linux/uaccess.h>
    #include <linux/jiffies.h>
	
#elif defined(WINCE)
    #include <windows.h>
    #include <drvmsg.h>
    
#else
	#include <stdio.h>   
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
* The slave address for I2C and SPI, the base address for EBI2.
*===========================================================================*/
#define RAONTV_CHIP_ADDR	0x86 

/*============================================================================
* Modifies the basic data types if neccessry.
*===========================================================================*/
typedef int					BOOL;
typedef signed char			S8;
typedef unsigned char		U8;
typedef signed short		S16;
typedef unsigned short		U16;
typedef signed int			S32;
typedef unsigned int		U32;

typedef int                 INT;
typedef unsigned int        UINT;
typedef long                LONG;
typedef unsigned long       ULONG;
 

typedef volatile U8			VU8;
typedef volatile U16		VU16;
typedef volatile U32		VU32;

#if defined(__GNUC__)
	#define INLINE		inline
#elif defined(_WIN32)
	#define INLINE		__inline
#elif defined(__ARMCC_VERSION)
	#define INLINE		__inline
#else
	/* Need to modified */
	#define INLINE		inline
#endif

/*============================================================================
* Selects the TV mode(s) to target product.
*===========================================================================*/
#define RTV_ISDBT_ENABLE

/*============================================================================
* Defines the package type of chip to target product.
*===========================================================================*/
#define RAONTV_CHIP_PKG_WLCSP	// MTV222
//#define RAONTV_CHIP_PKG_QFN	// MTV818A


/*============================================================================
* Defines the external source freqenecy in KHz.
* Ex> #define RTV_SRC_CLK_FREQ_KHz	36000 // 36MHz
*=============================================================================
* MTV250 : #define RTV_SRC_CLK_FREQ_KHz  32000  //must be defined 
* MTV350 : #define RTV_SRC_CLK_FREQ_KHz  19200  //must be defined 
*===========================================================================*/
//#define RTV_SRC_CLK_FREQ_KHz			36000
//#define RTV_SRC_CLK_FREQ_KHz			32000
#define RTV_SRC_CLK_FREQ_KHz			19200
	

/*============================================================================
* Define the power type.
*============================================================================*/  
//#define RTV_PWR_EXTERNAL
#define RTV_PWR_LDO


/*============================================================================
* Defines the Host interface.
*===========================================================================*/
//#define RTV_IF_MPEG2_SERIAL_TSIF // I2C + TSIF Master Mode. 
//#define RTV_IF_QUALCOMM_TSIF // I2C + TSIF Master Mode
#define RTV_IF_SPI // AP: SPI Master Mode
//#define RTV_IF_SPI_SLAVE // AP: SPI Slave Mode
//#define RTV_IF_EBI2 // External Bus Interface Slave Mode

/*#################################
# Pre-definintion by RAONTECH.
###################################*/
#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_QUALCOMM_TSIF)\
|| defined(RTV_IF_MPEG2_PARALLEL_TSIF)
	#define RTV_IF_TSIF /* All TSIF */
#endif

#if defined(RTV_IF_MPEG2_SERIAL_TSIF) || defined(RTV_IF_QUALCOMM_TSIF)
	#define RTV_IF_SERIAL_TSIF /* Serial TSIF */
#endif


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
	extern void mtv_delay_ms(int ms);
	#define RTV_DELAY_MS(ms) 	mtv_delay_ms(ms) // TODO
#endif

/*============================================================================
* Defines the debug message macro.
*===========================================================================*/
#if 1
	#define RTV_DBGMSG0(fmt)			printk(fmt)
	#define RTV_DBGMSG1(fmt, arg1)			printk(fmt, arg1) 
	#define RTV_DBGMSG2(fmt, arg1, arg2)		printk(fmt, arg1, arg2) 
	#define RTV_DBGMSG3(fmt, arg1, arg2, arg3)	printk(fmt, arg1, arg2, arg3) 
#else
	/* To eliminates the debug messages. */
	#define RTV_DBGMSG0(fmt)					((void)0) 
	#define RTV_DBGMSG1(fmt, arg1)				((void)0) 
	#define RTV_DBGMSG2(fmt, arg1, arg2)		((void)0) 
	#define RTV_DBGMSG3(fmt, arg1, arg2, arg3)	((void)0) 
#endif
/*#### End of Common ###########*/


/*############################################################################
#
# ISDB-T specific configurations
#
############################################################################*/


/* Determine if the output of error-tsp is disable. */
#define RTV_ERROR_TSP_OUTPUT_DISABLE

#ifndef RTV_ERROR_TSP_OUTPUT_DISABLE
	/* Determine if the NULL PID will generated for error-tsp. */
	//#define RTV_NULL_PID_GENERATE

#endif /* RTV_ERROR_TSP_OUTPUT_DISABLE */


/*============================================================================
* Defines the HRM ON setting Enable.
* In order to reject GSM/CDMA blocker, HRM ON must be defined.
*===========================================================================*/
#if defined(RTV_ISDBT_ENABLE)
	#ifdef RAONTV_CHIP_PKG_WLCSP
		#define RAONTV_CHIP_PKG_WLCSP_HRM_ON
	#endif
#endif

/*============================================================================
* Defines the polarity of interrupt if necessary.
*===========================================================================*/
#define RTV_INTR_POLARITY_LOW_ACTIVE
/* #define RTV_INTR_POLARITY_HIGH_ACTIVE */

/*############################################################################
#
# Host Interface specific configurations
#
############################################################################*/
extern U8 g_bRtvPage;

#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)
	/*=================================================================
	* Defines the TSIF interface for MPEG2 or QUALCOMM TSIF.	 
	*================================================================*/
	//#define RTV_TSIF_FORMAT_1
	//#define RTV_TSIF_FORMAT_2
	#define RTV_TSIF_FORMAT_3
	//#define RTV_TSIF_FORMAT_4
	//#define RTV_TSIF_FORMAT_5

	//#define RTV_TSIF_CLK_SPEED_DIV_2 // 12MHz
	//#define RTV_TSIF_CLK_SPEED_DIV_4 // 6MHz
	#define RTV_TSIF_CLK_SPEED_DIV_6 // 3MHz
	//#define RTV_TSIF_CLK_SPEED_DIV_8 // 1.5MHz

	/*=================================================================
	* Defines the register I/O macros.
	*================================================================*/
	unsigned char mtv_i2c_read(U8 reg);
	void mtv_i2c_read_burst(U8 reg, U8 *buf, int size);
	void mtv_i2c_write(U8 reg, U8 val);
	#define	RTV_REG_GET(reg)	mtv_i2c_read((U8)(reg))
	#define	RTV_REG_BURST_GET(reg, buf, size)	mtv_i2c_read_burst((U8)(reg), buf, size)
	#define	RTV_REG_SET(reg, val)	mtv_i2c_write((U8)(reg), (U8)(val))
	#define	RTV_REG_MASK_SET(reg, mask, val)\
		do {					\
			U8 tmp;				\
			tmp = (RTV_REG_GET(reg)|(U8)(mask)) & (U8)((~(mask))|(val));\
			RTV_REG_SET(reg, tmp);		\
		} while(0)

	#define RTV_TSP_XFER_SIZE	188

#elif defined(RTV_IF_SPI)
	/*=================================================================
	* Defines the register I/O macros.
	*================================================================*/
	unsigned char mtv222_spi_read(U8 page, U8 reg);
	void mtv222_spi_read_burst(U8 page, U8 reg, U8 *buf, int size);
	void mtv222_spi_write(U8 page, U8 reg, U8 val);

	#define	RTV_REG_GET(reg)			(U8)mtv222_spi_read(g_bRtvPage, (U8)(reg))
	#define	RTV_REG_BURST_GET(reg, buf, size)	mtv222_spi_read_burst(g_bRtvPage, (U8)(reg), buf, (size))
	#define	RTV_REG_SET(reg, val)			mtv222_spi_write(g_bRtvPage, (U8)(reg), (U8)(val))
	#define	RTV_REG_MASK_SET(reg, mask, val)\
		do {					\
			U8 tmp;				\
			tmp = (RTV_REG_GET(reg)|(U8)(mask)) & (U8)((~(mask))|(val));\
			RTV_REG_SET(reg, tmp);		\
		} while(0)

	#define RTV_TSP_XFER_SIZE	188

#elif defined(RTV_IF_EBI2)
	unsigned char mtv_ebi2_read(unsigned char reg);
	void mtv_ebi2_read_burst(unsigned char reg, unsigned char *buf, int size);
	void mtv_ebi2_write(unsigned char reg, unsigned char val);

#define	RTV_REG_GET(reg)  (U8)mtv_ebi2_read((U8)(reg))
#define	RTV_REG_BURST_GET(reg, buf, size) mtv_ebi2_read_burst((U8)(reg), buf, size)
#define	RTV_REG_SET(reg, val) mtv_ebi2_write((U8)(reg), (U8)(val))
#define	RTV_REG_MASK_SET(reg, mask, val) \
	do { \
		U8 tmp; \
		tmp = (RTV_REG_GET(reg)|(U8)(mask)) & (U8)((~(mask))|(val)); \
		RTV_REG_SET(reg, tmp); \
	} while (0)

	#define RTV_TSP_XFER_SIZE	188

#else
	#error "Must define the interface definition !"
#endif

/*############################################################################
#
# Pre-definintion by RAONTECH.
# Assume that FM only project was not exist.
#
############################################################################*/

#define RTV_SPI_MSC1_ENABLED /* to backward */

#if (defined(RTV_TDMB_ENABLE)||defined(RTV_DAB_ENABLE))\
&& !(defined(RTV_ISDBT_ENABLE)||defined(RTV_FM_ENABLE)) 
	/* Only TDMB or DAB enabled. */
	#define RTV_TDMBorDAB_ONLY_ENABLED

#elif !(defined(RTV_TDMB_ENABLE)||defined(RTV_DAB_ENABLE) || defined(RTV_FM_ENABLE))\
&& defined(RTV_ISDBT_ENABLE)
	/* Only 1SEG enabled. */
	#define RTV_ISDBT_ONLY_ENABLED

#elif (defined(RTV_TDMB_ENABLE)||defined(RTV_DAB_ENABLE)) && defined(RTV_FM_ENABLE)\
&& !defined(RTV_ISDBT_ENABLE)
		/* Only TDMB/DAB and FM  enabled. */
	#define RTV_TDMBorDAB_FM_ENABLED
#endif


/*############################################################################
# Define the critical object.
############################################################################*/
#if defined(RTV_IF_SPI) || defined(RTV_FIC_I2C_INTR_ENABLED)
	#if defined(__KERNEL__)	
		extern struct mutex raontv_guard;
		#define RTV_GUARD_INIT		mutex_init(&raontv_guard)
		#define RTV_GUARD_LOCK		mutex_lock(&raontv_guard)
		#define RTV_GUARD_FREE		mutex_unlock(&raontv_guard)
		#define RTV_GUARD_DEINIT 	((void)0)
		
    #elif defined(WINCE)        
	        extern CRITICAL_SECTION		raontv_guard;
	        #define RTV_GUARD_INIT		InitializeCriticalSection(&raontv_guard)
	        #define RTV_GUARD_LOCK		EnterCriticalSection(&raontv_guard)
	        #define RTV_GUARD_FREE		LeaveCriticalSection(&raontv_guard)
	        #define RTV_GUARD_DEINIT	DeleteCriticalSection(&raontv_guard)
	#else
		// temp: TODO
		#define RTV_GUARD_INIT		((void)0)
		#define RTV_GUARD_LOCK		((void)0)
		#define RTV_GUARD_FREE 	((void)0)
		#define RTV_GUARD_DEINIT 	((void)0)
	#endif
	
#else
	#define RTV_GUARD_INIT		((void)0)
	#define RTV_GUARD_LOCK		((void)0)
	#define RTV_GUARD_FREE 	((void)0)
	#define RTV_GUARD_DEINIT 	((void)0)
#endif


/*############################################################################
#
# Check erros by user-configurations.
#
############################################################################*/
#if !defined(RAONTV_CHIP_PKG_WLCSP) && !defined(RAONTV_CHIP_PKG_QFN)
	#error "Must define the package type !"
#endif

#if !defined(RTV_PWR_EXTERNAL) && !defined(RTV_PWR_LDO)
	#error "Must define the power type !"
#endif
 
#if defined(RTV_IF_TSIF) || defined(RTV_IF_SPI_SLAVE)\
|| defined(RTV_IF_SPI)
    #if (RAONTV_CHIP_ADDR >= 0xFF)
	#error "Invalid chip address"
    #endif

#elif defined(RTV_IF_EBI2)

#else
	#error "Must define the interface definition !"
#endif

void rtvOEM_ConfigureInterrupt(void);
void rtvOEM_PowerOn(int on);

#ifdef __cplusplus 
} 
#endif 

#endif /* __RAONTV_PORT_H__ */

