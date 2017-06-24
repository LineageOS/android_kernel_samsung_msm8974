/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fc8300_tun.c

	Description : source of FC8300 tuner driver

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
	0p1    initial driver
	0p2    VCO Currunt Setting Add
	0p4
	0p5    20130625
	0p6    20130628
	0p7    20130629
	0p8    20130704
	1p0    20130711
	1p0    20130723
	2p1    20130717
	2p7    20130723
	2p10   20130730
	2p11   20130812
	2p15   20130826
	2p16   20130827
	2p17   20130830
 ----------------------------------------------------------------------

*******************************************************************************/
#include "fci_types.h"
#include "fci_oal.h"
#include "fci_tun.h"
#include "fci_hal.h"
#include "fc8300_regs.h"
#include "fc8300_es1_tun.h"
#include "fc8300_es2_tun.h"
#include "fc8300_cs_tun.h"

#define FC8300_FREQ_XTAL BBM_XTAL_FREQ /* 19200 */
#define FC8300_TUN_ES1  0xa0
#define FC8300_TUN_ES2  0xb0
#define FC8300_TUN_CS   0xc0

#if 0
static s32 fc8300_write(HANDLE handle, DEVICEID devid, u8 addr, u8 data)
{
	s32 res;

	res = tuner_i2c_write(handle, devid, addr, 1, &data, 1);

	return res;
}
#endif

static s32 fc8300_read(HANDLE handle, DEVICEID devid, u8 addr, u8 *data)
{
	s32 res;

	res = tuner_i2c_read(handle, devid, addr, 1, data, 1);

	return res;
}

#if 0
static s32 fc8300_bb_write(HANDLE handle, DEVICEID devid, u16 addr, u8 data)
{
	s32 res;

	res = bbm_write(handle, devid, addr, data);

	return res;
}

static s32 fc8300_bb_read(HANDLE handle, DEVICEID devid, u16 addr, u8 *data)
{
	s32 res;

	res = bbm_read(handle, devid, addr, data);

	return res;
}
#endif

static u8 fc8300_tun_ver;

s32 fc8300_tuner_init(HANDLE handle, DEVICEID devid,
		enum BROADCAST_TYPE broadcast)
{
	fc8300_read(handle, devid, 0xff, &fc8300_tun_ver);

	if (fc8300_tun_ver == 0xa0)
		fc8300_tun_ver = FC8300_TUN_ES1;
	else if (fc8300_tun_ver == 0xb0)
		fc8300_tun_ver = FC8300_TUN_ES2;
	else if (fc8300_tun_ver == 0xc0)
		fc8300_tun_ver = FC8300_TUN_CS;

	if (fc8300_tun_ver == FC8300_TUN_ES1)
		return fc8300_es1_tuner_init(handle, devid, broadcast);
	else if (fc8300_tun_ver == FC8300_TUN_ES2)
		return fc8300_es2_tuner_init(handle, devid, broadcast);
	else if (fc8300_tun_ver == FC8300_TUN_CS)
		return fc8300_cs_tuner_init(handle, devid, broadcast);

	return BBM_OK;
}

s32 fc8300_set_freq(HANDLE handle, DEVICEID devid, u32 freq)
{
	if (fc8300_tun_ver == FC8300_TUN_ES1)
		return fc8300_es1_set_freq(handle, devid, freq);
	else if (fc8300_tun_ver == FC8300_TUN_ES2)
		return fc8300_es2_set_freq(handle, devid, freq);
	else if (fc8300_tun_ver == FC8300_TUN_CS)
		return fc8300_cs_set_freq(handle, devid, freq);

	return BBM_OK;
}

s32 fc8300_get_rssi(HANDLE handle, DEVICEID devid, s32 *rssi)
{
	if (fc8300_tun_ver == FC8300_TUN_ES1)
		return fc8300_es1_get_rssi(handle, devid, rssi);
	else if (fc8300_tun_ver == FC8300_TUN_ES2)
		return fc8300_es2_get_rssi(handle, devid, rssi);
	else if (fc8300_tun_ver == FC8300_TUN_CS)
		return fc8300_cs_get_rssi(handle, devid, rssi);

	return BBM_OK;
}

s32 fc8300_tuner_deinit(HANDLE handle, DEVICEID devid)
{
	if (fc8300_tun_ver == FC8300_TUN_ES1)
		return fc8300_es1_tuner_deinit(handle, devid);
	else if (fc8300_tun_ver == FC8300_TUN_ES2)
		return fc8300_es2_tuner_deinit(handle, devid);
	else if (fc8300_tun_ver == FC8300_TUN_CS)
		return fc8300_cs_tuner_deinit(handle, devid);

	return BBM_OK;
}

