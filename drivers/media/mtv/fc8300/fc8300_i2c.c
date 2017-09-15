/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300_i2c.c

	Description : source of I2C interface

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

	History :
	----------------------------------------------------------------------
*******************************************************************************/
#include "fci_types.h"
#include "fc8300_regs.h"
#include "fci_oal.h"
#include "fc8300_spi.h"

static s32 i2c_bulkread(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	return BBM_OK;
}

static s32 i2c_bulkwrite(HANDLE handle, u8 chip, u16 addr, u8 *data, u16 length)
{
	return BBM_OK;
}

s32 fc8300_i2c_init(HANDLE handle, u16 param1, u16 param2)
{
	OAL_CREATE_SEMAPHORE();

#ifdef BBM_I2C_SPI
	fc8300_spi_init(handle, 0, 0);
#else
	/* ts_initialize(); */
#endif

	return BBM_OK;
}

s32 fc8300_i2c_byteread(HANDLE handle, DEVICEID devid, u16 addr, u8 *data)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff, addr, data, 1);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_wordread(HANDLE handle, DEVICEID devid, u16 addr, u16 *data)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *) data, 2);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_longread(HANDLE handle, DEVICEID devid, u16 addr, u32 *data)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *) data, 4);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_bulkread(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u16 length)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_bytewrite(HANDLE handle, DEVICEID devid, u16 addr, u8 data)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *)&data, 1);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_wordwrite(HANDLE handle, DEVICEID devid, u16 addr, u16 data)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *)&data, 2);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_longwrite(HANDLE handle, DEVICEID devid, u16 addr, u32 data)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, (u8 *)&data, 4);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_bulkwrite(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u16 length)
{
	s32 res;

	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkwrite(handle, (u8) (devid >> 8) & 0xff,
			addr, data, length);
	OAL_RELEASE_SEMAPHORE();

	return res;
}

s32 fc8300_i2c_dataread(HANDLE handle, DEVICEID devid,
		u16 addr, u8 *data, u32 length)
{
	s32 res;

#ifdef BBM_I2C_SPI
	res = fc8300_spi_dataread(handle, devid,
		addr, data, length);
#else
	OAL_OBTAIN_SEMAPHORE();
	res = i2c_bulkread(handle, (u8) (devid >> 8) & 0xff,
			addr, data, length);
	OAL_RELEASE_SEMAPHORE();
#endif

	return res;
}

s32 fc8300_i2c_deinit(HANDLE handle)
{
#ifdef BBM_I2C_SPI
	fc8300_spi_deinit(handle);
#else
	/* ts_receiver_disable(); */
#endif

	OAL_DELETE_SEMAPHORE();

	return BBM_OK;
}

