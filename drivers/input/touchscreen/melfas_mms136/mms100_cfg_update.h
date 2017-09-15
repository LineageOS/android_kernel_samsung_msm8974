/*
 * include/linux/mms100_isc.h - ISC(In-system programming via I2C) enalbes MMS-100 Series sensor to be programmed while installed in a complete system.
 *
 * Copyright (C) 2012 Melfas, Inc.
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

#ifndef __MMS100_ISC_H__
#define __MMS100_ISC_H__

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/uaccess.h>

/*
 *       ISC Mode Porting Guide
 *           1. Check typedef
 *           2. Link I/O Port control code  (VDD, CE, INTR and etc)
 *           3. Define a delay function
 *               ex) Modify delay parameter constant to make it fit to your delay function such as mms100_usdelay() and mms100_msdelay() 
 *           5. Rename 'uart_printf()' to your console print function for debugging.
 *           6. Check Watchdog timer, Interrupt factor
 *           7. Copy mbin files to specific direction on your system
 */

/*
 *       Setup basic configuration of ISC mode
 */
#define MMS100_DEBUG_MSG_PRINT                      0

/*
 *       Configure watch dog API of host system to disable interrupts from other devices during ISC mode
 */
#define MMS100_DISABLE_BASEBAND_ISR()					disable_irq(OMAP_GPIO_IRQ(35))
#define MMS100_DISABLE_WATCHDOG_TIMER_RESET()		// Nothing
#define MMS100_ROLLBACK_BASEBAND_ISR()				enable_irq(OMAP_GPIO_IRQ(35))
#define MMS100_ROLLBACK_WATCHDOG_TIMER_RESET()		// Nothing
/*
 *       Configure I/O ports linked to touchscreen module in host system
 */
#if 0 // NOT USED 
#define MMS100_VDD_SET_HIGH()             			// specify a detailed information of host system
#define MMS100_VDD_SET_LOW()              			// specify a detailed information of host system
#define MMS100_CE_SET_HIGH()   	          			gpio_set_value(122, 1)
#define MMS100_CE_SET_LOW()      	        		gpio_set_value(122, 0)
#define MMS100_CE_SET_OUTPUT()   	        		// specify a detailed information of host system
#define MMS100_INTR_SET_HIGH()             	    		gpio_set_value(35, 1)
#define MMS100_INTR_SET_LOW()                  		gpio_set_value(35, 0)
#define MMS100_INTR_SET_OUTPUT()     		        // specify a detailed information of host system
#define MMS100_INTR_SET_INPUT()                		gpio_direction_input(35);
#define MMS100_GET_INTR()                       			gpio_get_value(35)
#endif 

/*
 *       Configure delay function interface to host system
 */
#define mms100_usdelay(x)                           do{(x) > (MAX_UDELAY_MS * 1000) ? mdelay((x)/1000) : udelay(x);}while(0) 
#define mms100_msdelay(x)                           mdelay(x) 

/*
 *       Configure I2C Interface to Linux system
 */
#if 0 // NOT USED
#define mms100_i2c_read(_client, _rd_buf, _len)   \
            i2c_master_recv(_client, _rd_buf, _len)
#define mms100_i2c_write(_client, _wr_buf, _len)    \
            i2c_master_send(_client, _wr_buf, _len)
#define mms100_read_i2c_block_data(_client, _wr_reg, _len, _rd_buf)     \
            i2c_smbus_read_i2c_block_data(_client, _wr_reg, _len, _rd_buf)
#endif 

/*
 *      Return values which is shown error message to the host system
 */
typedef enum {
	ISC_NONE = -1,
	ISC_SUCCESS = 0,
	ISC_FILE_OPEN_ERROR,
	ISC_FILE_CLOSE_ERROR,
	ISC_FILE_FORMAT_ERROR,
	ISC_WRITE_BUFFER_ERROR,
	ISC_I2C_ERROR,
	ISC_UPDATE_MODE_ENTER_ERROR,
	ISC_CRC_ERROR,
	ISC_VALIDATION_ERROR,
	ISC_COMPATIVILITY_ERROR,
	ISC_UPDATE_SECTION_ERROR,
	ISC_SLAVE_ERASE_ERROR,
	ISC_SLAVE_DOWNLOAD_ERROR,
	ISC_DOWNLOAD_WHEN_SLAVE_IS_UPDATED_ERROR,
	ISC_INITIAL_PACKET_ERROR,
	ISC_NO_NEED_UPDATE_ERROR,
	ISC_LIMIT
} eISCRet_t;

enum {
	BUILT_IN = 0,
	UMS,
};
/*
 *       Entry point of ISC functions to provide ISC feature to Linux system.
 */
eISCRet_t mms100_ISC_download_mbinary(struct i2c_client *_client, bool force_update, int fw_location);
#endif /* __MMS100_ISC_H__ */

