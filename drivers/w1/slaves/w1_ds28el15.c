/*
 * 1-Wire implementation for the ds23el15 chip
 *
 * Copyright (C) 2013 maximintergrated
 *
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/idr.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/err.h>
#include <linux/of.h>

#include <linux/input.h>

#include "../w1.h"
#include "../w1_int.h"
#include "../w1_family.h"
#include "w1_ds28el15_sha256.h"

// 1-Wire commands
#define CMD_WRITE_MEMORY         0x55
#define CMD_READ_MEMORY          0xF0
#define CMD_LOAD_LOCK_SECRET     0x33
#define CMD_COMPUTE_LOCK_SECRET  0x3C
#define CMD_SELECT_SECRET        0x0F
#define CMD_COMPUTE_PAGEMAC      0xA5
#define CMD_READ_STATUS          0xAA
#define CMD_WRITE_BLOCK_PROTECT  0xC3
#define CMD_WRITE_AUTH_MEMORY    0x5A
#define CMD_WRITE_AUTH_PROTECT   0xCC
#define CMD_PIO_READ             0xDD
#define CMD_PIO_WRITE            0x96
#define CMD_RELEASE		0xAA

#define BLOCK_READ_PROTECT       0x80
#define BLOCK_WRITE_PROTECT      0x40
#define BLOCK_EPROM_PROTECT      0x20
#define BLOCK_WRITE_AUTH_PROTECT 0x10

#define ROM_CMD_SKIP             0x3C
#define ROM_CMD_RESUME           0xA5

#define SELECT_SKIP     0
#define SELECT_RESUME   1
#define SELECT_MATCH    2
#define SELECT_ODMATCH  3
#define SELECT_SEARCH   4
#define SELECT_READROM  5
#define SELECT_ODSKIP	6

#define PROT_BIT_AUTHWRITE	0x10
#define PROT_BIT_EPROM		0x20
#define PROT_BIT_WRITE		0x40
#define PROT_BIT_READ		0x80

#define DS28E15_FAMILY		0x17

#define DS28E15_PAGES		2	// 2 pages, 4 blocks, 7 segment in each page. total 64 bytes
#define PAGE_TO_BYTE		32	// 1 page = 32 bytes
#define BLOCK_TO_BYTE		16	// 1 block = 16 bytes
#define SEGMENT_TO_BYTE		4	// 1 segment = 4 bytes

//#define LOW_VOLTAGE
#ifdef LOW_VOLTAGE
#define SHA_COMPUTATION_DELAY    4
#define EEPROM_WRITE_DELAY       15
#define SECRET_EEPROM_DELAY      200
#else
#define SHA_COMPUTATION_DELAY    3
#define EEPROM_WRITE_DELAY       10
#define SECRET_EEPROM_DELAY      90
#endif

#define ID_MIN		0
#define ID_MAX		3
#define CO_MIN		0
#ifdef CONFIG_SEC_MEGA2LTE_COMMON
#define CO_MAX		15
#else
#define CO_MAX		10
#endif
#define ID_DEFAULT	1
#define CO_DEFAULT	1
#define RETRY_LIMIT	10

#ifdef CONFIG_W1_CF
#define RETRY_LIMIT_CF	5
#endif
// misc state
static unsigned short slave_crc16;

static int special_mode = 0;
static char special_values[2];
static char rom_no[8];

int verification = -1, id = 2, color;
#ifdef CONFIG_W1_SN
char g_sn[14];
#endif
#ifdef CONFIG_W1_CF
int cf_node = -1;
#endif
#ifdef CONFIG_SEC_H_PROJECT
extern int verified;
#endif

#define READ_EOP_BYTE(seg) (32-seg*4)

static char w1_array[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static u8 skip_setup = 1;	// for test if the chip did not have secret code, we would need to write temp secret value
static u8 init_verify = 1;	// for inital verifying


//module_param_array(w1_array, char, NULL, 0);
//-----------------------------------------------------------------------------
// ------ DS28EL15 Functions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------
// Set or clear special mode flag
//
// 'enable'      - '1' to enable special mode or '0' to clear
//
void set_special_mode(int enable, uchar *values)
{
   special_mode= enable;
   special_values[0] = values[0];
   special_values[1] = values[1];
}

//--------------------------------------------------------------------------
// Calculate a new CRC16 from the input data shorteger.  Return the current
// CRC16 and also update the global variable CRC16.
//
static short oddparity[16] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0 };

static unsigned short docrc16(unsigned short data)
{
	data = (data ^ (slave_crc16 & 0xff)) & 0xff;
	slave_crc16 >>= 8;

	if (oddparity[data & 0xf] ^ oddparity[data >> 4])
        slave_crc16 ^= 0xc001;

	data <<= 6;
	slave_crc16  ^= data;
	data <<= 1;
	slave_crc16   ^= data;

	return slave_crc16;
}

//--------------------------------------------------------------------------
//  Compute MAC to write a 4 byte memory block using an authenticated
//  write.
//
//  Parameters
//     page - page number where the block to write is located (0 to 15)
//     segment - segment number in page (0 to 7)
//     new_data - 4 byte buffer containing the data to write
//     old_data - 4 byte buffer containing the data to write
//     manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//     mac - buffer to put the calculated mac into
//
//  Returns: TRUE - mac calculated
//           FALSE - Failed to calculate
//
int calculate_write_authMAC256(int page, int segment, char *new_data, char *old_data, char *manid, char *mac)
{
	char mt[64];

	// calculate MAC
	// clear
	memset(mt,0,64);

	// insert ROM number
	memcpy(&mt[32],rom_no,8);

	mt[43] = segment;
	mt[42] = page;
	mt[41] = manid[0];
	mt[40] = manid[1];

	// insert old data
	memcpy(&mt[44],old_data,4);

	// insert new data
	memcpy(&mt[48],new_data,4);

	// compute the mac
	return compute_mac256(mt, 55, &mac[0]);
}


//-----------------------------------------------------------------------------
// ------ DS28EL15 Functions - 1 wire command
//-----------------------------------------------------------------------------
//--------------------------------------------------------------------------
//  Write a 4 byte memory block. The block location is selected by the
//  page number and offset blcok within the page. Multiple blocks can
//  be programmed without re-selecting the device using the continue flag.
//  This function does not use the Authenticated Write operation.
//
//  Parameters
//     page - page number where the block to write is located (0, 1)
//     segment - segment number in page (0 to 7)
//     data - 4 byte buffer containing the data to write
//     contflag - Flag to indicate the write is continued from the last (=1)
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_write_block(struct w1_slave *sl, int page, int seg, uchar *data, int contflag)
{
	uchar buf[256],cs;
	int cnt, i, offset;
	int length =4;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -ENODEV;

	if (!contflag)
	{
		if (w1_reset_select_slave(sl))
			return -1;

		buf[cnt++] = CMD_WRITE_MEMORY;
		buf[cnt++] = (seg << 5) | page;   // address

		// Send command
		w1_write_block(sl->master, &buf[0], 2);

		// Read CRC
		w1_read_block(sl->master, &buf[cnt], 2);

		cnt += 2;

		offset = cnt;
	}

	// add the data
	for (i = 0; i < length; i++)
		buf[cnt++] = data[i];

	// Send data
	w1_write_block(sl->master, data, length);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check the first CRC16
	if (!contflag)
	{
		slave_crc16 = 0;
		for (i = 0; i < offset; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001)
			return -1;
	}

	// check the second CRC16
	slave_crc16 = 0;
	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for EEPROM writing.
	msleep(EEPROM_WRITE_DELAY);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;

}

//--------------------------------------------------------------------------
//  Write a memory segment. The segment location is selected by the
//  page number and offset segment within the page. Multiple segments can
//  be programmed without re-selecting the device.
//  This function does not use the Authenticated Write operation.
//
//  Parameters
//     page - page number where the block to write is located (0, 1)
//     seg - segment number in page (0 to 7)
//     data - 4 byte multiple buffer containing the data to write
//     length - length to write (4 multiple number, 4, 8, ..., 32)
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_write_memory(struct w1_slave *sl, int seg, int page, uchar *data, int length)
{
	uchar buf[256];
	uchar cs=0;
	int i;

	if (!sl)
		return -ENODEV;

	// program one or more contiguous 4 byte segments of a memory block.
	if (length%4)
		return -EINVAL;


	memcpy(buf,data,length);

	for (i=0;i<length/4;i++) {
		cs = w1_ds28el15_write_block(sl, page, i, &buf[i*4], (i==0)? 0 : 1);
	}

	return cs;
}

//--------------------------------------------------------------------------
//  Write a 4 byte memory block using an authenticated write (with MAC).
//  The block location is selected by the
//  page number and offset blcok within the page. Multiple blocks can
//  be programmed without re-selecting the device using the continue flag.
//  This function does not use the Authenticated Write operation.
//
//  Parameters
//     page - page number where the block to write is located (0, 1)
//     segment - segment number in page (0 to 7)
//     data - SHA input data for the Authenticated Write Memory including new_data, old_data, and manid
//	new_data - 4 byte buffer containing the data to write
//	old_data - 4 byte buffer containing the data to write
//	manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//
//  Restrictions
//     The memory block containing the targeted 4-byte segment must not be write protected.
//     The Read/Write Scratchpad command(w1_ds28el15_write_scratchpad) must have been issued once
//     in write mode after power-on reset to ensure proper setup of the SHA-256 engine.
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_write_authblock(struct w1_slave *sl, int page, int segment, uchar *data, int contflag)
{
	uchar buf[256],cs;
	uchar new_data[4], old_data[4], manid[2];
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -ENODEV;

	memcpy(new_data, &data[0], 4);
	memcpy(old_data, &data[4], 4);
	memcpy(manid, &data[8], 2);

	if (!contflag) {
		if (w1_reset_select_slave(sl))
			return -1;

		buf[cnt++] = CMD_WRITE_AUTH_MEMORY;
		buf[cnt++] = (segment << 5) | page;   // address

		// Send command
		w1_write_block(sl->master, &buf[0], 2);

		// Read the first CRC
		w1_read_block(sl->master, &buf[cnt], 2);
		cnt += 2;

		offset = cnt;
	}

	// add the data
	for (i = 0; i < 4; i++)
		buf[cnt++] = new_data[i];

	// Send data - first 4bytes
	w1_write_block(sl->master, new_data, 4);

	// read the second CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// now wait for the MAC computation.
	msleep(SHA_COMPUTATION_DELAY);

	if (!contflag) {
		// check the first CRC16
		slave_crc16 = 0;
		for (i = 0; i < offset; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001)
			return -1;
	}

	// check the second CRC16
	slave_crc16 = 0;

	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// compute the mac
	if (special_mode)
	{
		if (!calculate_write_authMAC256(page, segment, new_data, old_data, special_values, &buf[0]))
			return -1;
	}
	else
	{
		if (!calculate_write_authMAC256(page, segment, new_data, old_data, manid, &buf[0]))
		return -1;
	}

	// transmit MAC as a block - send the second 32bytes
	cnt=0;
	w1_write_block(sl->master, buf, 32);

	// calculate CRC on MAC
	slave_crc16 = 0;
	for (i = 0; i < 32; i++)
		docrc16(buf[i]);

	// append read of CRC16 and CS byte
	w1_read_block(sl->master, &buf[0], 3);
	cnt = 3;

	// ckeck CRC16
	for (i = 0; i < (cnt-1); i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check CS
	if (buf[cnt-1] != 0xAA)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	msleep(EEPROM_WRITE_DELAY);

	// disable strong pullup


	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;

}

//--------------------------------------------------------------------------
//  Write a 4 byte memory block using an authenticated write (with MAC).
//  The MAC must be pre-calculated.
//
//  Parameters
//     page - page number where the block to write is located (0 to 15)
//     segment - segment number in page (0 to 7)
//     new_data - 4 byte buffer containing the data to write
//     mac - mac to use for the write
//
//  Returns: 0 - block written
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_write_authblockMAC(struct w1_slave *sl, int page, int segment, uchar *new_data, uchar *mac)
{
	uchar buf[256],cs;
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -ENODEV;

	// check if not continuing a previous block write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_WRITE_AUTH_MEMORY;
	buf[cnt++] = (segment << 5) | page;   // address

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// add the data
	for (i = 0; i < 4; i++)
		buf[cnt++] = new_data[i];

	// Send data
	w1_write_block(sl->master, new_data, 4);

	// read first CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// now wait for the MAC computation.
	msleep(SHA_COMPUTATION_DELAY);

	// disable strong pullup

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check the second CRC16
	slave_crc16 = 0;

	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// transmit MAC as a block
	w1_write_block(sl->master, mac, 32);

	// calculate CRC on MAC
	slave_crc16 = 0;
	for (i = 0; i < 32; i++)
		docrc16(mac[i]);

	// append read of CRC16 and CS byte
	w1_read_block(sl->master, &buf[0], 3);
	cnt = 3;

	// ckeck CRC16
	for (i = 0; i < (cnt-1); i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check CS
	if (buf[cnt-1] != 0xAA)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	msleep(EEPROM_WRITE_DELAY);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}

#ifdef CONFIG_W1_CF
//--------------------------------------------------------------------------
//  Read memory. Multiple pages can
//  be read without re-selecting the device using the continue flag.
//
//  Parameters
//	 seg - segment number(0~7) in page
//     page - page number where the block to read is located (0 to 15)
//     rdbuf - 32 byte buffer to contain the data to read
//     length - length to read (allow jsut segment(4bytes) unit) (4, 8, 16, ... , 64)
//
//  Returns: 0 - block read and verified CRC
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_read_memory_check(struct w1_slave *sl, int seg, int page, uchar *rdbuf, int length)
{
	uchar buf[256];
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -ENODEV;

	// Check presence detect
	if (w1_reset_overdrive_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_READ_MEMORY;
	buf[cnt++] = (seg<<5) | page;	 // address

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// read data and CRC16
	w1_read_block(sl->master, &buf[cnt], length+2);
	cnt+=length+2;

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -2;

	if (READ_EOP_BYTE(seg) == length) {
		// check the second CRC16
		slave_crc16 = 0;


		for (i = offset; i < cnt; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001)
			return -2;
	}

	// copy the data to the read buffer
	memcpy(rdbuf,&buf[offset],length);

	return 0;
}
#endif

//--------------------------------------------------------------------------
//  Read memory. Multiple pages can
//  be read without re-selecting the device using the continue flag.
//
//  Parameters
//	 seg - segment number(0~7) in page
//     page - page number where the block to read is located (0 to 15)
//     rdbuf - 32 byte buffer to contain the data to read
//     length - length to read (allow jsut segment(4bytes) unit) (4, 8, 16, ... , 64)
//
//  Returns: 0 - block read and verified CRC
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_read_memory(struct w1_slave *sl, int seg, int page, uchar *rdbuf, int length)
{
	uchar buf[256];
	int cnt, i, offset;

	cnt = 0;
	offset = 0;

	if (!sl)
		return -ENODEV;

	// Check presence detect
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_READ_MEMORY;
	buf[cnt++] = (seg<<5) | page;	 // address

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// read data and CRC16
	w1_read_block(sl->master, &buf[cnt], length+2);
	cnt+=length+2;

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	if (READ_EOP_BYTE(seg) == length) {
		// check the second CRC16
		slave_crc16 = 0;


		for (i = offset; i < cnt; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001)
			return -1;
	}

	// copy the data to the read buffer
	memcpy(rdbuf,&buf[offset],length);

	return 0;
}


//--------------------------------------------------------------------------
//  Read page and verify CRC. Multiple pages can
//  be read without re-selecting the device using the continue flag.
//
//  Parameters
//     page - page number where the block to write is located (0 to 15)
//     rdbuf - 32 byte buffer to contain the data to read
//
//  Returns: 0 - block read and verified CRC
//               else - Failed to write block (no presence or invalid CRC16)
//
int w1_ds28el15_read_page(struct w1_slave *sl, int page, uchar *rdbuf)
{
	return w1_ds28el15_read_memory(sl, 0, page, rdbuf, 32);
}

//----------------------------------------------------------------------
// Read the scratchpad (challenge or secret)
//
// 'rbbuf'      - 32 byte buffer to contain the data to read
//
// Return: 0 - select complete
//             else - error during select, device not present
//
int w1_ds28el15_read_scratchpad(struct w1_slave *sl, uchar *rdbuf)
{
	uchar buf[256];
	int cnt=0, i, offset;

	if (!sl)
		return -ENODEV;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_SELECT_SECRET;
	buf[cnt++] = 0x0F;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// read data and CRC16
	w1_read_block(sl->master, &buf[cnt], 34);
	cnt+=34;

	// check first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check the second CRC16
	slave_crc16 = 0;
	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// copy the data to the read buffer
	memcpy(rdbuf,&buf[offset],32);

	return 0;
}

//----------------------------------------------------------------------
// Write the scratchpad (challenge or secret)
//
// 'data'      - data to write to the scratchpad (32 bytes)
//
// Return: 0 - select complete
//             else - error during select, device not present
//
int w1_ds28el15_write_scratchpad(struct w1_slave *sl, uchar *data)
{
	uchar buf[256];
	int cnt=0, i, offset;

	if (!sl)
		return -ENODEV;


	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_SELECT_SECRET;
	buf[cnt++] = 0x00;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	offset = cnt;

	// add the data
	memcpy(&buf[cnt], data, 32);
	cnt+=32;

	// Send the data
	w1_write_block(sl->master, data, 32);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -2;

	// check the second CRC16
	slave_crc16 = 0;
	for (i = offset; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -3;

	return 0;
}

//----------------------------------------------------------------------
// Load first secret operation on the DS28E25/DS28E22/DS28E15.
//
// 'lock'      - option to lock the secret after the load (lock = 1)
//
// Restrictions
//             The Read/Write Scratchpad command(w1_ds28el15_write_scratchpad) must have been issued
//             in write mode prior to Load and Lock Secret to define the secret's value.
//
// Return: 0 - load complete
//             else - error during load, device not present
//
int w1_ds28el15_load_secret(struct w1_slave *sl, int lock)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -ENODEV;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_LOAD_LOCK_SECRET;
	buf[cnt++] = (lock) ? 0xE0 : 0x00;  // lock flag

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	msleep(SECRET_EEPROM_DELAY);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}


//----------------------------------------------------------------------
// Compute secret operation on the DS28E25/DS28E22/DS28E15.
//
// 'partial'   - partial secret to load (32 bytes)
// 'pbyte' - parameter byte including page_num and lock
//	'page_num' - page number to read 0 - 16
//	'lock'      - option to lock the secret after the load (lock = 1)
//
//  Restrictions
//     The Read/Write Scratchpad command(w1_ds28el15_write_scratchpad) must have been issued
//     in write mode prior to Compute and Lock Secret to define the partial secret.
//
// Return: 0 - compute complete
//		  else - error during compute, device not present
//
int w1_ds28el15_compute_secret(struct w1_slave *sl, int pbyte)
{
	uchar buf[256],cs;
	int cnt=0, i;
	int page_num, lock;

	if (!sl)
		return -ENODEV;

	page_num = pbyte & 0x01;
	lock = (pbyte & 0xE0) ? 1 : 0;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_COMPUTE_LOCK_SECRET;
	buf[cnt++] = (lock) ? (0xE0 | page_num) : page_num;  // lock flag

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// send release and strong pull-up
	buf[0] = CMD_RELEASE;
	w1_write_block(sl->master, &buf[0], 1);

	// now wait for the MAC computation.
	msleep(SHA_COMPUTATION_DELAY * 2 + SECRET_EEPROM_DELAY);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}


//--------------------------------------------------------------------------
//  Do Compute Page MAC command and return MAC. Optionally do
//  annonymous mode (anon != 0).
//
//  Parameters
//     pbyte - parameter byte including page_num and anon
//	page_num - page number to read 0, 1
//	anon - Flag to indicate Annonymous mode if (anon != 0)
//     mac - 32 byte buffer for page data read
//
//  Restrictions
//     The Read/Write Scratchpad command(w1_ds28el15_write_scratchpad) must have been issued
//     in write mode prior to Compute and Read Page MAC to define the challenge.
//
//  Returns: 0 - page read has correct MAC
//               else - Failed to read page or incorrect MAC
//
int w1_ds28el15_compute_read_pageMAC(struct w1_slave *sl, int pbyte, uchar *mac)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -ENODEV;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_COMPUTE_PAGEMAC;
	buf[cnt++] = pbyte;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -2;

	// now wait for the MAC computation.
	msleep(SHA_COMPUTATION_DELAY * 2);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);
	if (cs != 0xAA)
		return -3;

	// read the MAC and CRC
	w1_read_block(sl->master, &buf[0], 34);

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < 34; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -4;

	// copy MAC to return buffer
	memcpy(mac, buf, 32);

	return 0;
}

//--------------------------------------------------------------------------
//  Do Read Athenticated Page command and verify MAC. Optionally do
//  annonymous mode (anon != 0).
//
//  Parameters
//     page_num - page number to read 0, 1
//     challange - 32 byte buffer containing the challenge
//     mac - 32 byte buffer for mac read
//     page_data - 32 byte buffer to contain the data to read
//     manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//     skipread - Skip the read page and use the provided data in the 'page_data' buffer
//     anon - Flag to indicate Annonymous mode if (anon != 0)
//
//  Returns: 0 - page read has correct MAC
//               else - Failed to read page or incorrect MAC
//
int w1_ds28el15_read_authverify(struct w1_slave *sl, int page_num, uchar *challenge, uchar *page_data, uchar *manid, int skipread, int anon)
{
	uchar mac[32];
	uchar mt[128];
	int pbyte;
	int i = 0, rslt;

	if (!sl)
		return -ENODEV;

	if(w1_array[0] == 0)	return 0;

	// check to see if we skip the read (use page_data)
	if (!skipread)
	{
		// read the page to get data
		while (i < RETRY_LIMIT) {
			rslt = w1_ds28el15_read_page(sl, page_num, page_data);
			if (rslt == 0)
				break;

			mdelay(10); /* wait 10ms */
			i++;
		}
		if (i > RETRY_LIMIT) {
			printk(KERN_ERR "w1_ds28el15_read_authverify err :  -1F\n");
			return -1;
		}
		i = 0;
	}
	// The Read/Write Scratch pad command must have been issued in write mode prior
	// to Compute and Read Page MAC to define the challenge
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28el15_write_scratchpad(sl, challenge);
		if (rslt == 0)
			break;

		mdelay(10); /* wait 10ms */
		i++;
	}
	if (i > RETRY_LIMIT) {
		printk(KERN_ERR "w1_ds28el15_read_authverify err :  -2F\n");
		return -2;
	}
	i = 0;

	// have device compute mac
	pbyte = anon ? 0xE0 : 0x00;
	pbyte = pbyte | page_num;
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28el15_compute_read_pageMAC(sl, pbyte, mac);
		if (rslt == 0)
			break;

		mdelay(10); /* wait 10ms */
		i++;
	}
	if (i > RETRY_LIMIT) {
		printk(KERN_ERR "w1_ds28el15_read_authverify err :  -3\n");
		return -3;
	}
	// create buffer to compute and verify mac

	// clear
	memset(mt,0,128);

	// insert page data
	memcpy(&mt[0],page_data,32);

	// insert challenge
	memcpy(&mt[32],challenge,32);

	// insert ROM number or FF
	if (anon)
		memset(&mt[96],0xFF,8);
	else
		memcpy(&mt[96],rom_no,8);

	mt[106] = page_num;

	if (special_mode)
	{
		mt[105] = special_values[0];
		mt[104] = special_values[1];
	}
	else
	{
		mt[105] = manid[0];
		mt[104] = manid[1];
	}

	if (verify_mac256(mt, 119, mac) ==0) {
		printk(KERN_ERR "w1_ds28el15_compute_read_pageMAC err :  -4\n");
		return -4;
	} else
		return 0;
}


//--------------------------------------------------------------------------
//  Verify provided MAC and page data. Optionally do
//  annonymous mode (anon != 0).
//
//  Parameters
//     page_num - page number to read 0 - 16
//     challange - 32 byte buffer containing the challenge
//     page_data - 32 byte buffer to contain the data read
//     manid - 2 byte buffer containing the manufacturer ID (general device: 00h,00h)
//     mac - 32 byte buffer of mac read
//     anon - Flag to indicate Annonymous mode if (anon != 0)
//
//  Returns: 0 - page read has correct MAC
//               else - Failed to read page or incorrect MAC
//
int w1_ds28el15_authverify(struct w1_slave *sl, int page_num, uchar *challenge, uchar *page_data, uchar *manid, uchar *mac, int anon)
{
	uchar mt[128];

	// create buffer to compute and verify mac

	// clear
	memset(mt,0,128);

	// insert page data
	memcpy(&mt[0],page_data,32);

	// insert challenge
	memcpy(&mt[32],challenge,32);

	// insert ROM number or FF
	if (anon)
		memset(&mt[96],0xFF,8);
	else
		memcpy(&mt[96],rom_no,8);

	mt[106] = page_num;

	if (special_mode)
	{
		mt[105] = special_values[0];
		mt[104] = special_values[1];
	}
	else
	{
		mt[105] = manid[0];
		mt[104] = manid[1];
	}

	if (verify_mac256(mt, 119, mac) == 0)
		return -1;
	else
		return 0;
}

//--------------------------------------------------------------------------
//  Read status bytes, either personality or page protection.
//
//  Parameters
//     pbyte - include personality, allpages, and page_num
//	personality - flag to indicate the read is the 4 personality bytes (1)
//		      or page page protection (0)
//	allpages - flag to indicate if just one page (0) or all (1) page protection
//		      bytes.
//	block_num - block number if reading protection 0 to 3
//     rdbuf - 16 byte buffer personality bytes (length 4) or page protection
//            (length 1 or 16)
//
//  Returns: 0 - status read
//               else - Failed to read status
//
int w1_ds28el15_read_status(struct w1_slave *sl, int pbyte, uchar *rdbuf)
{
	uchar buf[256];
	int cnt, i, offset,rdnum;
	int personality, allpages, block_num;

	if (!sl)
		return -ENODEV;

	personality = (pbyte & 0xE0) ? 1 : 0;
	allpages = (pbyte == 0) ? 1 : 0;
	block_num = pbyte & 0x03;

	cnt = 0;
	offset = 0;

	if (w1_reset_select_slave(sl)){
		pr_info("%s reset_select_slave error", __func__);
		return -1;
	}

	buf[cnt++] = CMD_READ_STATUS;
	if (personality)
		buf[cnt++] = 0xE0;
	else if (!allpages)
		buf[cnt++] = block_num;
	else
		buf[cnt++] = 0;

	// send the command
	w1_write_block(sl->master, &buf[0], 2);
	offset = cnt + 2;

	// adjust data length
	if ((personality) || (allpages))
		rdnum = 8;
	else
		rdnum = 5;

	// Read the bytes
	w1_read_block(sl->master, &buf[cnt], rdnum);
	cnt += rdnum;

	// check the first CRC16
	slave_crc16 = 0;
	for (i = 0; i < offset; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001){
		pr_info("%s slave_crc16 is error 1.\n", __func__);
		return -1;
	}

	if ((personality || allpages || (block_num == 1)))
	{
		// check the second CRC16
		slave_crc16 = 0;
		for (i = offset; i < cnt; i++)
			docrc16(buf[i]);

		if (slave_crc16 != 0xB001){
			pr_info("%s slave_crc16 is error 2.\n", __func__);
			return -1;
		}
	}

	// copy the data to the read buffer
	memcpy(rdbuf,&buf[offset],rdnum-4);
	return 0;
}

//--------------------------------------------------------------------------
//  Write page protection byte.
//
//  Parameters
//     block - block number (0 to 3) which covers two pages each
//     prot - protection byte
//
//  Returns: 0 - protection written
//               else - Failed to set protection
//
int w1_ds28el15_write_blockprotection(struct w1_slave *sl, uchar prot)
{
	uchar buf[256],cs;
	int cnt=0, i;

	if (!sl)
		return -ENODEV;

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_WRITE_BLOCK_PROTECT;

	// compute parameter byte
	buf[cnt++] = prot;

	w1_write_block(sl->master, &buf[0], cnt);

	// Read CRC
	w1_read_block(sl->master, &buf[cnt], 2);
	cnt += 2;

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// sent release

	// now wait for programming
	msleep(EEPROM_WRITE_DELAY);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}

//--------------------------------------------------------------------------
//  Write page protection byte.
//
//  Parameters
//     data - input data for Authenticated Write Block Protection
//	new_value - new protection byte(parameter byte) including block num
//	old_value - old protection byte(parameter byte)
//	manid - manufacturer ID
//
//  Returns: 0 - protection written
//               else - Failed to set protection
//
int w1_ds28el15_write_authblockprotection(struct w1_slave *sl, uchar *data)
{
	uchar buf[256],cs,mt[64];
	int cnt=0, i;
	int new_value, old_value;
	uchar manid[2];

	if (!sl)
		return -ENODEV;

	new_value = data[0];
	old_value = data[1];
	manid[0] = data[2];
	manid[1] = data[3];

	// select device for write
	if (w1_reset_select_slave(sl))
		return -1;

	buf[cnt++] = CMD_WRITE_AUTH_PROTECT;
	buf[cnt++] = new_value;

	// Send command
	w1_write_block(sl->master, &buf[0], 2);

	// read first CRC
	w1_read_block(sl->master, &buf[cnt], 2);

	// now wait for the MAC computation.
	msleep(SHA_COMPUTATION_DELAY);

	// disable strong pullup

	// check CRC16
	slave_crc16 = 0;
	for (i = 0; i < cnt; i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// calculate MAC
	// clear
	memset(mt,0,64);

	// insert ROM number
	memcpy(&mt[32],rom_no,8);

	// instert block and page
	mt[43] = 0;
	mt[42] = new_value & 0x0F;

	// check on special mode
	if (special_mode)
	{
		mt[41] = special_values[0];
		mt[40] = special_values[1];
	}
	else
	{
		mt[41] = manid[0];
		mt[40] = manid[1];
	}

	// old data
	mt[44] = (old_value & PROT_BIT_AUTHWRITE) ? 0x01 : 0x00;
	mt[45] = (old_value & PROT_BIT_EPROM) ? 0x01 : 0x00;
	mt[46] = (old_value & PROT_BIT_WRITE) ? 0x01 : 0x00;
	mt[47] = (old_value & PROT_BIT_READ) ? 0x01 : 0x00;
	// new data
	mt[48] = (new_value & PROT_BIT_AUTHWRITE) ? 0x01 : 0x00;
	mt[49] = (new_value & PROT_BIT_EPROM) ? 0x01 : 0x00;
	mt[50] = (new_value & PROT_BIT_WRITE) ? 0x01 : 0x00;
	mt[51] = (new_value & PROT_BIT_READ) ? 0x01 : 0x00;

	// compute the mac
	compute_mac256(mt, 55, &buf[0]);
	cnt = 32;

	// send the MAC
	w1_write_block(sl->master, &buf[0], 32);

	// Read CRC and CS byte
	w1_read_block(sl->master, &buf[cnt], 3);
	cnt += 3;

	// ckeck CRC16
	slave_crc16 = 0;
	for (i = 0; i < (cnt-1); i++)
		docrc16(buf[i]);

	if (slave_crc16 != 0xB001)
		return -1;

	// check CS
	if (buf[cnt-1] != 0xAA)
		return -1;

	// send release and strong pull-up
	// DATASHEET_CORRECTION - last bit in release is a read-zero so don't check echo of write byte
	w1_write_8(sl->master, 0xAA);

	// now wait for the MAC computation.
	msleep(EEPROM_WRITE_DELAY);

	// disable strong pullup

	// read the CS byte
	cs = w1_read_8(sl->master);

	if (cs == 0xAA)
		return 0;
	else
		return cs;
}

//-------------------------------------------------------------------------
// get_array_value
//
#ifdef CONFIG_OF_SUBCMDLINE_PARSE
static int get_array_value(void)
{
	int i, ret;
	char str[20], *buf,  data;

	pr_info("%s: W1 Read Array.\n", __func__);
	ret = of_parse_args_on_subcmdline("array=",(char *)str);
	if (ret) {
		pr_err("%s: W1 Read Array Error\n", __func__);
		return -1;
	}
	buf = str;
	for(i=0;i<5;i++){
		strncpy(&data,&buf[i*2],2);
		sscanf(&data, "%x", (unsigned int *)&ret);
		w1_array[i]=ret;
	}
	return 0;
}
#else
static int __init get_array_value(char *str)
{
	int i, get[6];

	pr_info("%s: W1 Read Array\n", __func__);

	for(i=0;i<5;i++){
		sscanf(str, "%x", &get[i]);
		w1_array[i]=get[i];
		str=str+3;
	}
	return 0;
}
__setup("array=", get_array_value);
#endif


//--------------------------------------------------------------------------
//  w1_ds28el15_verifymac
//
//  compare two mac data, Device mac and calculated mac and
//  returned the result
//  Returns: 0 - Success, have the same mac data in both of Device and AP.
//               else - Failed, have the different mac data or verifying sequnce failed.
//
int w1_ds28el15_verifymac(struct w1_slave *sl)
{
	int rslt,rt;
	uchar buf[256], challenge[32], manid[2];
	uchar memimage[512];
	uchar master_secret[32];
#if 1
	int i = 0;
#endif

	// copy the secret code
	memcpy(master_secret, w1_array, 32); // Set temp_secret to the master secret

	rt = 0;

	if(w1_array[0] == 0) {
		pr_info("%s: w1_array[0] == 0\n", __func__);
		goto success;
	}

	// Store the master secret and romid.
	set_secret(master_secret);
	set_romid(rom_no);

	// read personality bytes to get manufacturer ID
#if 1
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28el15_read_status(sl, 0xE0, buf);
		if (rslt == 0)
			break;

		mdelay(10); /* wait 10ms */
		i++;
	}
	i = 0;
#else
	rslt = w1_ds28el15_read_status(sl, 0xE0, buf);
#endif
	if (rslt == 0)
	{
		manid[0] = buf[3];
		manid[1] = buf[2];
	} else {
		pr_info("%s : read_status error\n", __func__);
		rt = -1;
		goto success;
	}

	// if you want to use random value, insert code here.
	get_random_bytes(challenge, 32);  // random challenge

	// Enhance verification status
	while (i < RETRY_LIMIT) {
		rslt = w1_ds28el15_read_authverify(sl, 0, challenge,
					&memimage[0], manid, 0, 0);
		if (rslt == 0)
			break;

		mdelay(10); /* wait 10ms */
		i++;
	}
	printk(KERN_ERR "%s  result : %d\n",__func__, rslt);

	if (rslt){
		pr_info("%s: read_authverify error\n", __func__);
		rt = -1;
	}

success:
	return rt;
}

static int w1_ds28el15_setup_device(struct w1_slave *sl)
{
	int rslt,rt;
	uchar buf[256], challenge[32], manid[2];
	uchar memimage[512];
	uchar master_secret[32];

	// hard code master secret
	memcpy(master_secret, w1_array, 32); // Set temp_secret to the master secret

	// ----- DS28EL25/DS28EL22/DS28EL15 Setup
	printk(KERN_ERR "-------- DS28EL15 Setup Example\n");
	rt = 0;

	printk(KERN_ERR "Write scratchpad - master secret\n");
	rslt = w1_ds28el15_write_scratchpad(sl, master_secret);
	printk(KERN_ERR "result : %d\n", rslt);
	if (rslt) rt = -1;

	printk(KERN_ERR "Load the master secret\n");
	rslt = w1_ds28el15_load_secret(sl, 0);
	printk(KERN_ERR "result : %d\n",rslt);
	if (rslt) rt = -1;

	printk(KERN_ERR "Set the master secret in the Software SHA-256\n");
	set_secret(master_secret);
	set_romid(rom_no);

	// fill image with random data to write
	// read personality bytes to get manufacturer ID
	printk(KERN_ERR " w1_ds28el15_read_status(\n");
	rslt = w1_ds28el15_read_status(sl, 0xE0, buf);
	if (rslt == 0) {
		manid[0] = buf[3];
		manid[1] = buf[2];
	}
	else{
		rt = -1;
		goto end;
	}
	printk(KERN_ERR "result : %d\n",rslt);

	printk(KERN_ERR "Read-Authenticate with unique secret\n");
	get_random_bytes(challenge, 32);  // random challenge
	rslt = w1_ds28el15_read_authverify(sl, 0, challenge, &memimage[0], manid, 0, 0);
	printk(KERN_ERR "result : %d\n",rslt);
	if (rslt) rt = -1;

	printk(KERN_ERR "DS28EL15 Setup Example: %s\n",(rt) ? "FAIL" : "SUCCESS");
	printk(KERN_ERR "--------------------------------------------------\n");
end:
	return rt;

}

#if 0
static int w1_ds28el15_application(struct w1_slave *sl)
{
	int i,rslt,rt;
	uchar buf[256], challenge[32], manid[2], new_page[32];
	uchar memimage[512];
	uchar master_secret[32];

	// ----- DS28EL25/DS28EL22/DS28EL15 Application Example
	printk(KERN_ERR "\n-------- DS28EL15 Application Example\n");

	// hard code master secret
	for (i = 0; i < 32; i++)
		master_secret[i] = w1_array[i];  // Set master secret to temp_secret

	rt = 0;

	printk(KERN_ERR "Set the master secret in the Software SHA-256\n");
	set_secret(master_secret);
	set_romid(rom_no);

	// read personality bytes to get manufacturer ID
	rslt = w1_ds28el15_read_status(sl, 0xE0, buf);
	if (rslt == 0)
	{
		manid[0] = buf[3];
		manid[1] = buf[2];
	}
	else
		rt = -1;
	printk(KERN_ERR "result : %d\n",rslt);

	printk(KERN_ERR "Read-Authenticated page 0\n");
	get_random_bytes(challenge, 32);  // random challenge
	rslt = w1_ds28el15_read_authverify(sl, 0, challenge, &memimage[0], manid, 0, 0);
	printk(KERN_ERR "result : %d\n",rslt);
	if (rslt) rt = -1;

	// change data block 0 by inverting the current data
	printk(KERN_ERR "Old data: ");
	for (i = 0; i < 4; i++)
		printk(KERN_ERR "%02X ",memimage[i]);

	printk(KERN_ERR "\nNew data: ");
	for (i = 0; i < 4; i++)
	{
		new_page[i] = ~memimage[i];
		printk(KERN_ERR "%02X ",new_page[i]);
	}

	// Write new block
	printk(KERN_ERR "\nWrite block at address 0000h\n");
	memset(&buf[0], 0x00, 10);	// use 10bytes(4+4+2) for data
	memcpy(&buf[0], &new_page[0], 4);
	memcpy(&buf[4], &memimage[0], 4);
	memcpy(&buf[8], manid, 2);
	rslt = w1_ds28el15_write_authblock(sl, 0, 0, buf, 0);
	printk(KERN_ERR "result : %d\n",rslt);
	if (rslt) rt = -1;
	memcpy(&memimage[0], &new_page[0], 4);

	// verify memory is correct by doing just a compute
	printk(KERN_ERR "Read-Authenticate page 0 after write\n");
	get_random_bytes(challenge, 32);  // random challenge
	rslt = w1_ds28el15_read_authverify(sl, 0, challenge, &memimage[0], manid, 1, 0);
	printk(KERN_ERR "result : %d\n",rslt);
	if (rslt) rt = -1;

	printk(KERN_ERR "DS28EL15 Application Example: %s\n",(rt) ? "FAIL" : "SUCCESS");
	printk(KERN_ERR "--------------------------------------------------------\n");

	return rt;
}
#endif



static ssize_t w1_ds28el15_read_user_eeprom(struct device *device,
	struct device_attribute *attr, char *buf);

static struct device_attribute w1_read_user_eeprom_attr =
	__ATTR(read_eeprom, S_IRUGO, w1_ds28el15_read_user_eeprom, NULL);

// read 64 bytes user eeprom data
static ssize_t w1_ds28el15_read_user_eeprom(struct device *device,
	struct device_attribute *attr, char *buf)
{
	struct w1_slave *sl = dev_to_w1_slave(device);
	u8 rdbuf[64];
	int result, i, page_num;
	ssize_t c = PAGE_SIZE;

	// read page 0
	page_num=0;
	c -= snprintf(buf + PAGE_SIZE - c, c, "page %d read\n", page_num);
	result = w1_ds28el15_read_page(sl, page_num, &rdbuf[0]);
	if (result != 0) {
		c -= snprintf(buf + PAGE_SIZE - c, c, "\npage %d read fail\n", page_num);
		return 0;
	}
	for (i = 0; i < 32; ++i)
		c -= snprintf(buf + PAGE_SIZE - c, c, "%02x ", rdbuf[i]);
	c -= snprintf(buf + PAGE_SIZE - c, c, "\npage %d read done\n", page_num);

	// read page 1
	page_num=1;
	result = w1_ds28el15_read_page(sl, page_num, &rdbuf[32]);
	if (result != 0) {
		c -= snprintf(buf + PAGE_SIZE - c, c, "\npage %d read fail\n", page_num);
		return 0;
	}
	for (i = 32; i < 64; ++i)
		c -= snprintf(buf + PAGE_SIZE - c, c, "%02x ", rdbuf[i]);
	c -= snprintf(buf + PAGE_SIZE - c, c, "\npage %d read done\n", page_num);

	return PAGE_SIZE - c;
}

static ssize_t w1_ds28el15_check_color(struct device *device,
	struct device_attribute *attr, char *buf);

static struct device_attribute w1_check_color_attr =
	__ATTR(check_color, S_IRUGO, w1_ds28el15_check_color, NULL);

// check COLOR for cover
static ssize_t w1_ds28el15_check_color(struct device *device,
	struct device_attribute *attr, char *buf)
{
	// read cover color
	return sprintf(buf, "%d\n", color);
}

#ifdef CONFIG_W1_CF
static ssize_t w1_ds28el15_cf(struct device *device,
	struct device_attribute *attr, char *buf);

static struct device_attribute w1_cf_attr =
	__ATTR(cf, S_IRUGO, w1_ds28el15_cf, NULL);

// check cf_node cover or not
static ssize_t w1_ds28el15_cf(struct device *device,
	struct device_attribute *attr, char *buf)
{
	// read mem
	return sprintf(buf, "%d\n", cf_node);
}
#endif

static ssize_t w1_ds28el15_check_id(struct device *device,
	struct device_attribute *attr, char *buf);

static struct device_attribute w1_check_id_attr =
	__ATTR(check_id, S_IRUGO, w1_ds28el15_check_id, NULL);

// check ID for cover
static ssize_t w1_ds28el15_check_id(struct device *device,
	struct device_attribute *attr, char *buf)
{
	// read cover id
	return sprintf(buf, "%d\n", id);
}

static ssize_t w1_ds28el15_verify_mac(struct device *device,
	struct device_attribute *attr, char *buf);

static struct device_attribute w1_verifymac_attr =
	__ATTR(verify_mac, S_IRUGO, w1_ds28el15_verify_mac, NULL);

// verified mac value between the device and AP
static ssize_t w1_ds28el15_verify_mac(struct device *device,
	struct device_attribute *attr, char *buf)
{
	struct w1_slave *sl = dev_to_w1_slave(device);
	int result;

	// verify mac
	result = w1_ds28el15_verifymac(sl);
	return sprintf(buf, "%d\n", result);
}

static int w1_ds28el15_get_buffer(struct w1_slave *sl, uchar *rdbuf, int retry_limit)
{
	int ret = -1, retry = 0;

	while((ret != 0) && (retry < retry_limit)) {
		ret = w1_ds28el15_read_page(sl, 0, &rdbuf[0]);
		if(ret != 0 )
			pr_info("%s : error %d\n", __func__, ret);

		retry++;
	}

	return ret;
}

#ifdef CONFIG_W1_SN
static const int sn_cdigit[19] = {
	0x0e, 0x0d, 0x1f, 0x0b, 0x1c,
	0x12, 0x0f, 0x1e, 0x0a, 0x13,
	0x14, 0x15, 0x19, 0x16, 0x17,
	0x20, 0x1b, 0x1d, 0x11};

static bool w1_ds28el15_check_digit(const uchar *sn)
{
	int i, tmp1 = 0, tmp2 = 0;
	int cdigit = sn[3];

	if (cdigit == 0x1e)
		return true;

	for (i=4;i<10;i++)
		tmp1 += sn[i];

	tmp1 += sn[4]*5;
	tmp2 = (tmp1 * sn[9] * sn[13]) % 19;

	tmp1 = (sn[10] + sn[12]) * 3 + (sn[11] + sn[13]) * 6 + 14;

	if (cdigit == sn_cdigit[((tmp1 + tmp2) % 19)])
		return true;
	else
		return false;
}

static uchar w1_ds28el15_char_convert(uchar c)
{
	char ctable[36] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K',
	'L', 'M', 'N', 'P', 'Q', 'R', 'S', 'T', 'V', 'W',
	'X', 'Y', 'Z', 'I', 'O', 'U'};

	return ctable[c];
}

static void w1_ds28el15_slave_sn(const uchar *rdbuf)
{
	int i;
	u8 sn[15];

	sn[14] = 0;

	if (w1_ds28el15_check_digit(&rdbuf[4])) {
		for (i = 0 ; i < 14 ; i++)
			sn[i] = w1_ds28el15_char_convert(rdbuf[i+4]);

		pr_info("%s: %s\n", __func__, sn);

		for (i = 0 ; i < 14 ; i++)
			g_sn[i] = sn[13 - i];
	} else {
		for (i = 0 ; i < 14 ; i++)
			sn[i] = w1_ds28el15_char_convert(rdbuf[i+4]);

		pr_info("%s: sn is not good %s\n", __func__, sn);
	}
}
#endif

static void w1_ds28el15_update_slave_info(struct w1_slave *sl) {
	u8 rdbuf[32];
	int ret, retry = 0;

	ret = w1_ds28el15_get_buffer(sl, &rdbuf[0], 10);
	if(ret != 0)
		pr_info("%s : fail to get buffer %d\n", __func__, ret);

	while((rdbuf[0] < sl->id_min) || (rdbuf[0] > sl->id_max) ||
		(rdbuf[1] < sl->color_min) || (rdbuf[1] > sl->color_max)) {
		printk(KERN_ERR "%s: out of range : %d %d \n", __func__, rdbuf[0], rdbuf[1]);

		if(retry > 10) {
			printk(KERN_ERR "%s: out of range over 10 times.\n", __func__);

			pr_info("%s: Change ID(%d) & Color(%d) to Default Value(%d, %d)",
				__func__, rdbuf[0], rdbuf[1], sl->id_default, sl->color_default);
			rdbuf[0] = sl->id_default;
			rdbuf[1] = sl->color_default;

			break;
		}
		ret = w1_ds28el15_get_buffer(sl, &rdbuf[0], 10);
		if(ret != 0)
			pr_info("%s : fail to get buffer %d\n", __func__, ret);

		retry++;
	}

	//read page 0, ID is 0 ~ 7 bit(rdbuf[0])
	id = rdbuf[0];

	// read page 0, COLOR is 8 ~ 15 bit(rdbuf[1])
	color = rdbuf[1];

	pr_info("%s Read ID(%d) & Color(%d) & Verification State(%d)\n",
			__func__, id, color, verification);

#ifdef CONFIG_W1_SN
	w1_ds28el15_slave_sn(&rdbuf[0]);
#endif
}
static int w1_parse_dt(struct w1_slave *sl)
{
	struct device_node* np;

	np = of_find_node_by_path("/soc/ds28el15");
	if (!np) {
		pr_err("%s: get ds28el15 node failed\n", __func__);
		return -ENODEV;
	}
	of_property_read_u32(np, "ds28el15,id-min", &sl->id_min);
	of_property_read_u32(np, "ds28el15,id-max", &sl->id_max);
	of_property_read_u32(np, "ds28el15,id-default", &sl->id_default);
	of_property_read_u32(np, "ds28el15,color-min", &sl->color_min);
	of_property_read_u32(np, "ds28el15,color-max", &sl->color_max);
	of_property_read_u32(np, "ds28el15,color-default", &sl->color_default);
	pr_info("%s : id min[%d] max[%d] default[%d]\n", __func__,
			sl->id_min, sl->id_max, sl->id_default);
	pr_info("%s : color min[%d] max[%d] default[%d]\n", __func__,
			sl->color_min, sl->color_max, sl->color_default);

	return 0;
}

static void w1_set_default_range(struct w1_slave *sl)
{
	pr_info("%s : get default range\n", __func__);

	sl->id_min = ID_MIN;
	sl->id_max = ID_MAX;
	sl->id_default = ID_DEFAULT;
	sl->color_min = CO_MIN;
	sl->color_max = CO_MAX;
	sl->color_default = CO_DEFAULT;

	pr_info("%s : id min[%d] max[%d] default[%d]\n", __func__,
			sl->id_min, sl->id_max, sl->id_default);
	pr_info("%s : color min[%d] max[%d] default[%d]\n", __func__,
			sl->color_min, sl->color_max, sl->color_default);
}

static int w1_ds28el15_add_slave(struct w1_slave *sl)
{
	int err = 0;
#ifdef CONFIG_W1_CF
	int count = 0, rst = 0;
	u8 rdbuf[32];
#endif

	printk(KERN_ERR "\nw1_ds28el15_add_slave start\n");

	err = w1_parse_dt(sl);
	if (err) {
		printk(KERN_ERR "%s: w1_parse_dt error\n", __func__);
		w1_set_default_range(sl);
	}
#ifdef CONFIG_OF_SUBCMDLINE_PARSE
	err = get_array_value();
	if (err) {
		printk(KERN_ERR "%s: w1_get_array_value error\n", __func__);
	}
#endif
	err = device_create_file(&sl->dev, &w1_read_user_eeprom_attr);
	if (err) {
		device_remove_file(&sl->dev, &w1_read_user_eeprom_attr);
		printk(KERN_ERR "%s: w1_read_user_eeprom_attr error\n", __func__);
		return err;
	}

	err = device_create_file(&sl->dev, &w1_verifymac_attr);
	if (err) {
		device_remove_file(&sl->dev, &w1_verifymac_attr);
		printk(KERN_ERR "%s: w1_verifymac_attr error\n", __func__);
		return err;
	}

#ifdef CONFIG_W1_CF
	err = device_create_file(&sl->dev, &w1_cf_attr);
	if (err) {
		device_remove_file(&sl->dev, &w1_cf_attr);
		printk(KERN_ERR "%s: w1_cf_attr error\n", __func__);
		return err;
	}
#endif

	err = device_create_file(&sl->dev, &w1_check_id_attr);
	if (err) {
		device_remove_file(&sl->dev, &w1_check_id_attr);
		printk(KERN_ERR "%s: w1_check_id_attr error\n", __func__);
		return err;
	}

	err = device_create_file(&sl->dev, &w1_check_color_attr);
	if (err) {
		device_remove_file(&sl->dev, &w1_check_color_attr);
		printk(KERN_ERR "%s: w1_check_color_attr error\n", __func__);
		return err;
	}

	// copy rom id to use mac calculation
	memcpy(rom_no, (u8 *)&sl->reg_num, sizeof(sl->reg_num));

	if (init_verify) {
		if (skip_setup == 0) {
			err = w1_ds28el15_setup_device(sl);
			printk(KERN_ERR "w1_ds28el15_setup_device\n");
			skip_setup = 1;
			err = w1_ds28el15_verifymac(sl);
			verification = err;
		} else {
			err = w1_ds28el15_verifymac(sl);
			verification = err;
			printk(KERN_ERR "w1_ds28el15_verifymac\n");
		}
	}
#ifdef CONFIG_SEC_H_PROJECT
	pr_info("%s:verified(%d)", __func__, verified);
	if(!verified)
#else
	if(!verification)
#endif
	{
#ifdef CONFIG_W1_CF
		while (count < RETRY_LIMIT_CF) {
			rst = w1_ds28el15_read_memory_check(sl, 0, 0, rdbuf, 32);
			if (rst == 0)
				break;
			count++;
		}
		if (rst == -2)
			cf_node = 1;
		else
			cf_node = 0;

		pr_info("%s:COVER CLASS(%d)\n", __func__, cf_node);
#endif
#if defined(CONFIG_SEC_H_PROJECT)
		w1_ds28el15_update_slave_info(sl);
#else
		w1_ds28el15_update_slave_info(sl);
		pr_info("%s:uevent send 1\n", __func__);
		input_report_switch(sl->master->bus_master->input, SW_W1, 1);
		input_sync(sl->master->bus_master->input);
#endif
	}

	printk(KERN_ERR "w1_ds28el15_add_slave end, skip_setup=%d, err=%d\n", skip_setup, err);
	return err;
}

static void w1_ds28el15_remove_slave(struct w1_slave *sl)
{
	device_remove_file(&sl->dev, &w1_read_user_eeprom_attr);
	device_remove_file(&sl->dev, &w1_verifymac_attr);
#ifdef CONFIG_W1_CF
	device_remove_file(&sl->dev, &w1_cf_attr);
#endif
	device_remove_file(&sl->dev, &w1_check_id_attr);
	device_remove_file(&sl->dev, &w1_check_color_attr);

	verification = -1;
	printk(KERN_ERR "\nw1_ds28el15_remove_slave\n");
}

static struct w1_family_ops w1_ds28el15_fops = {
	.add_slave      = w1_ds28el15_add_slave,
	.remove_slave   = w1_ds28el15_remove_slave,
};

static struct w1_family w1_ds28el15_family = {
	.fid = W1_FAMILY_DS28EL15,
	.fops = &w1_ds28el15_fops,
};

static int __init w1_ds28el15_init(void)
{
	return w1_register_family(&w1_ds28el15_family);
}

static void __exit w1_ds28el15_exit(void)
{
	w1_unregister_family(&w1_ds28el15_family);
}

late_initcall(w1_ds28el15_init);
module_exit(w1_ds28el15_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clark Kim <clark.kim@maximintegrated.com>");
MODULE_DESCRIPTION("1-wire Driver for Maxim/Dallas DS23EL15 DeepCover Secure Authenticator IC");
