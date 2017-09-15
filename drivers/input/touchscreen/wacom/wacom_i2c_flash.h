/*
 *  wacom_i2c_flash.h - Wacom G5 Digitizer Controller (I2C bus)
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "wacom_i2c_func.h"
#include "wacom_i2c_firm.h"
#include <linux/firmware.h>

#define WACOM_I2C_MODE_BOOT	1
#define WACOM_I2C_MODE_NORMAL	0

#define FLASH_START0 'f'
#define FLASH_START1 'l'
#define FLASH_START2 'a'
#define FLASH_START3 's'
#define FLASH_START4 'h'
#define FLASH_START5 '\r'
#define FLASH_ACK    0x06

#define PANA_QUERY   0x11

#define FLASH_END         0x80
#define FLASH_VERIFY      0x81
#define FLASH_WRITE       0x82
#define FLASH_READ        0x83
#define FLASH_ERASE       0x84
#define FLASH_SET_INFO    0x85
#define FLASH_END_TO_BOOT 0x87
#define FLASH_BAUDRATE    0x88

#define FLASH_QUERY    0xE0
#define FLASH_BLVER    0xE1
#define FLASH_UNITID   0xE2
#define FLASH_GET_INFO 0xE3
#define FLASH_FWVER    0xE4
#define FLASH_MPU      0xE8

#define WRITE_BUFF       300
#define BLOCK_SIZE_W     128
#define NUM_BLOCK_2WRITE 16
#define BANK             0
#define START_ADDR       0x1000
#define END_BLOCK        4

#define MAX_BLOCK_W8501  31
#define MPUVER_W8501     0x26
#define BLVER_W8501      0x41
#define MAX_ADDR_W8501   0x7FFF

#define MAX_BLOCK_514    47
#define MPUVER_514       0x27
#define BLVER_514        0x50
#define MAX_ADDR_514     0xBFFF

#define MPUVER_505             0x28
#define MAX_BLOCK_505          59
#define MAX_ADDR_505           0xEFFF
#define BLVER_505              0xFF

#define RETRY            1

#define ERR_FAILED_ENTER -1
#define ERR_UNSENT       -2
#define ERR_NOT_FLASH    -3
#define ERR_FAILED_EXIT  -4

#define PEN_QUERY '*'

#define START_ADDR_W9007	0x2000
#define MAX_ADDR_W9007		0xfbff
#define BLOCK_NUM_W9007 	62

#define CMD_GET_FEATURE	         2
#define CMD_SET_FEATURE	         3

#define MPU_W9001	0x28
#define MPU_W9007	0x2A
#define MPU_W9010	0x2C

#define FLASH_BLOCK_SIZE	64

/*#define WRITE         0*/
#define VERIFY		1
#define WRITEVERIFY	2
#define ERASE		3
#define GETVERSION	4

#define USER_ADDRESS	0x56
#define BOOT_ADDRESS	0x57

//++
#define ACK				0

#define BOOT_CMD_SIZE	78
#define BOOT_RSP_SIZE	6

#define BOOT_CMD_REPORT_ID	7

#define BOOT_ERASE_FLASH		0
#define BOOT_WRITE_FLASH		1
#define BOOT_VERIFY_FLASH		2
#define BOOT_EXIT				3
#define BOOT_BLVER				4
#define BOOT_MPU				5
#define BOOT_SECURITY_UNLOCK	6
#define BOOT_QUERY				7

#define QUERY_CMD 0x07
#define QUERY_ECH 'D'
#define QUERY_RSP 0x06

#define BOOT_CMD 0x04
#define BOOT_ECH 'D'

#define MPU_CMD 0x05
#define MPU_ECH 'D'

#define SEC_CMD 0x06
#define SEC_ECH 'D'
#define SEC_RSP 0x00

#define ERS_CMD 0x00
#define ERS_ECH 'D'
#define ERS_RSP 0x00

#define MARK_CMD 0x02
#define MARK_ECH 'D'
#define MARK_RSP 0x00

#define WRITE_CMD 0x01
#define WRITE_ECH 'D'
#define WRITE_RSP 0x00

#define VERIFY_CMD 0x02
#define VERIFY_ECH 'D'
#define VERIFY_RSP 0x00
//--
//++
#define CMD_SIZE (72+6)
#define RSP_SIZE 6
//--
#define DATA_SIZE (65536 * 2)

#define FIRM_VER_LB_ADDR_W9007	0xFBFE
#define FIRM_VER_UB_ADDR_W9007	0xFBFF	
#define FIRM_VER_LB_ADDR_W9001	0xEFFE
#define FIRM_VER_UB_ADDR_W9001	0xEFFF

/* EXIT_RETURN_VALUE */
enum {
	EXIT_OK = 0,
	EXIT_REBOOT,
	EXIT_FAIL,
	EXIT_USAGE,
	EXIT_NO_SUCH_FILE,
	EXIT_NO_INTEL_HEX,
	EXIT_FAIL_OPEN_COM_PORT,
	EXIT_FAIL_ENTER_FLASH_MODE,
	EXIT_FAIL_FLASH_QUERY,
	EXIT_FAIL_BAUDRATE_CHANGE,
	EXIT_FAIL_WRITE_FIRMWARE,
	EXIT_FAIL_EXIT_FLASH_MODE,
	EXIT_CANCEL_UPDATE,
	EXIT_SUCCESS_UPDATE,
	EXIT_FAIL_HID2SERIAL,
	EXIT_FAIL_VERIFY_FIRMWARE,
	EXIT_FAIL_MAKE_WRITING_MARK,
	EXIT_FAIL_ERASE_WRITING_MARK,
	EXIT_FAIL_READ_WRITING_MARK,
	EXIT_EXIST_MARKING,
	EXIT_FAIL_MISMATCHING,
	EXIT_FAIL_ERASE,
	EXIT_FAIL_GET_BOOT_LOADER_VERSION,
	EXIT_FAIL_GET_MPU_TYPE,
	EXIT_MISMATCH_BOOTLOADER,
	EXIT_MISMATCH_MPUTYPE,
	EXIT_FAIL_ERASE_BOOT,
	EXIT_FAIL_WRITE_BOOTLOADER,
	EXIT_FAIL_SWAP_BOOT,
	EXIT_FAIL_WRITE_DATA,
	EXIT_FAIL_GET_FIRMWARE_VERSION,
	EXIT_FAIL_GET_UNIT_ID,
	EXIT_FAIL_SEND_STOP_COMMAND,
	EXIT_FAIL_SEND_QUERY_COMMAND,
	EXIT_NOT_FILE_FOR_535,
	EXIT_NOT_FILE_FOR_514,
	EXIT_NOT_FILE_FOR_503,
	EXIT_MISMATCH_MPU_TYPE,
	EXIT_NOT_FILE_FOR_515,
	EXIT_NOT_FILE_FOR_1024,
	EXIT_FAIL_VERIFY_WRITING_MARK,
	EXIT_DEVICE_NOT_FOUND,
	EXIT_FAIL_WRITING_MARK_NOT_SET,
	EXIT_FAIL_SET_PDCT,
	ERR_SET_PDCT,
	ERR_GET_PDCT,
	ERR_SET_PDCT_IRQ,
};

extern int wacom_i2c_flash_chksum(struct wacom_i2c *wac_i2c,
			unsigned char *flash_data,
			unsigned long *max_address);
extern int wacom_i2c_flash_cmd(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_query(struct wacom_i2c *wac_i2c,
			u8 query, u8 recvdQuery);
extern int wacom_i2c_flash_end(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_enter(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_BLVer(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_mucId(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_erase(struct wacom_i2c *wac_i2c,
			u8 cmd_erase, u8 cmd_block, u8 endBlock);
extern int wacom_i2c_flash_write(struct wacom_i2c *wac_i2c,
			unsigned long startAddr, u8 size,
			unsigned long maxAddr);
extern int wacom_i2c_flash_verify(struct wacom_i2c *wac_i2c,
			unsigned long startAddr, u8 size,
			unsigned long maxAddr);
extern int wacom_i2c_flash(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_9007(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_flash_fw_check(struct wacom_i2c *wac_i2c);
extern int wacom_check_mpu_version(struct wacom_i2c *wac_i2c);
extern int wacom_i2c_select_flash_code(struct wacom_i2c *wac_i2c);
extern int wacom_fw_load_from_UMS(struct wacom_i2c *wac_i2c);
extern int wacom_load_fw_from_req_fw(struct wacom_i2c *wac_i2c);
extern int wacom_enter_bootloader(struct wacom_i2c *wac_i2c);
extern int wacom_check_flash_mode(struct wacom_i2c *wac_i2c, int mode);
