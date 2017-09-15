/*****************************************************************************
	Copyright(c) 2009 FCI Inc. All Rights Reserved

	File name : fc8050_ppi.c

	Description : fc8050 host interface

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
	2009/09/14	jason		initial
*******************************************************************************/
#include <linux/spinlock.h>
#include "fci_types.h"
#include "fc8050_regs.h"
#include "fc8050_ppi.h"
#include <linux/io.h>

static DEFINE_SPINLOCK(fci_lock);

#ifdef CONFIG_TDMB_EBI
u32 base_address;
#define BBM_BASE_ADDR  (void __iomem *)base_address
#define BBM_BASE_OFFSET			0x00
#else
#define BBM_BASE_ADDR				0x00
#define BBM_BASE_OFFSET			0x00
#endif

#define PPI_BMODE                       0x00
#define PPI_WMODE                       0x10
#define PPI_LMODE                       0x20
#define PPI_READ                        0x40
#define PPI_WRITE                       0x00
#define PPI_AINC                        0x80


int fc8050_ppi_init(HANDLE hDevice, u16 param1, u16 param2)
{
	base_address = param2;
	base_address <<= 16;
	base_address |= param1;

	DPRINTK("%s : 0x%p\n", __func__, (void __iomem *)base_address);

	return BBM_OK;
}

int fc8050_ppi_byteread(HANDLE hDevice, u16 addr, u8 *data)
{
	u16 length = 1;
	unsigned long flags;

	spin_lock_irqsave(&fci_lock, flags);
	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8((PPI_READ | ((length & 0x0f00) >> 8)), BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	*data = ioread8(BBM_BASE_ADDR);
	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_wordread(HANDLE hDevice, u16 addr, u16 *data)
{
	u16 length = 2;
	u8 command = PPI_AINC | PPI_READ | PPI_BMODE;

	unsigned long flags;

	spin_lock_irqsave(&fci_lock, flags);
	if (BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
		command = PPI_READ | PPI_WMODE;

	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8((command | ((length & 0x0f00) >> 8)), BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	*data = ioread8(BBM_BASE_ADDR);
	*data |= ioread8(BBM_BASE_ADDR) << 8;
	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_longread(HANDLE hDevice, u16 addr, u32 *data)
{
	u16 length = 4;
	unsigned long flags;
	u8 command = (PPI_AINC | PPI_READ | ((length & 0x0f00) >> 8));

	spin_lock_irqsave(&fci_lock, flags);
	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8(command, BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	*data = ioread8(BBM_BASE_ADDR);
	*data |= ioread8(BBM_BASE_ADDR) << 8;
	*data |= ioread8(BBM_BASE_ADDR) << 16;
	*data |= ioread8(BBM_BASE_ADDR) << 24;
	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_bulkread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int i;
	unsigned long flags;
	u8 command = (PPI_AINC | PPI_READ | ((length & 0x0f00) >> 8));

	spin_lock_irqsave(&fci_lock, flags);
	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8(command, BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	for (i = 0; i < length; i++)
		data[i] = ioread8(BBM_BASE_ADDR);

	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_bytewrite(HANDLE hDevice, u16 addr, u8 data)
{
	u16 length = 1;
	unsigned long flags;

	spin_lock_irqsave(&fci_lock, flags);
	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8((PPI_WRITE | ((length & 0x0f00) >> 8)), BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	iowrite8(data, BBM_BASE_ADDR);
	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_wordwrite(HANDLE hDevice, u16 addr, u16 data)
{
	u16 length = 2;
	u8 command = PPI_AINC | PPI_WRITE | PPI_BMODE;

	unsigned long flags;

	spin_lock_irqsave(&fci_lock, flags);
	if (BBM_SCI_DATA <= addr && BBM_SCI_SYNCRX >= addr)
		command = PPI_WRITE | PPI_WMODE;

	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8((command | ((length & 0x0f00) >> 8)), BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	iowrite8((data & 0xff), BBM_BASE_ADDR);
	iowrite8(((data & 0xff00) >> 8), BBM_BASE_ADDR);

	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_longwrite(HANDLE hDevice, u16 addr, u32 data)
{
	u16 length = 4;
	unsigned long flags;
	u8 command = PPI_AINC | PPI_WRITE | ((length & 0x0f00) >> 8);

	spin_lock_irqsave(&fci_lock, flags);
	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8(command, BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	iowrite8((data &  0x000000ff), BBM_BASE_ADDR);
	iowrite8(((data & 0x0000ff00) >> 8), BBM_BASE_ADDR);
	iowrite8(((data & 0x00ff0000) >> 16), BBM_BASE_ADDR);
	iowrite8(((data & 0xff000000) >> 24), BBM_BASE_ADDR);

	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_bulkwrite(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int i;
	unsigned long flags;
	u8 command = PPI_AINC | PPI_WRITE | ((length & 0x0f00) >> 8);

	spin_lock_irqsave(&fci_lock, flags);
	iowrite8((addr & 0xff), BBM_BASE_ADDR);
	iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
	iowrite8(command, BBM_BASE_ADDR);
	iowrite8((length & 0xff), BBM_BASE_ADDR);

	for (i = 0; i < length; i++)
		iowrite8(data[i], BBM_BASE_ADDR);

	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_dataread(HANDLE hDevice, u16 addr, u8 *data, u16 length)
{
	int i, j;
	u16 x, y;
	unsigned long flags;

	x = length / 4095;
	y = length % 4095;


	spin_lock_irqsave(&fci_lock, flags);

	for (i = 0; i < x; i++) {
		iowrite8((addr & 0xff), BBM_BASE_ADDR);
		iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
		iowrite8((PPI_READ | ((4095 & 0x0f00) >> 8)), BBM_BASE_ADDR);
		iowrite8((4095 & 0xff), BBM_BASE_ADDR);

		for (j = 0; j < 4095; j++)
			data[4095*i+j] = ioread8(BBM_BASE_ADDR);
	}

	if (y) {
		iowrite8((addr & 0xff), BBM_BASE_ADDR);
		iowrite8(((addr & 0xff00) >> 8), BBM_BASE_ADDR);
		iowrite8((PPI_READ | ((y & 0x0f00) >> 8)), BBM_BASE_ADDR);
		iowrite8((y & 0xff), BBM_BASE_ADDR);

		for (j = 0; j < y; j++)
			data[4095*x+j] = ioread8(BBM_BASE_ADDR);
	}
	spin_unlock_irqrestore(&fci_lock, flags);

	return BBM_OK;
}

int fc8050_ppi_deinit(HANDLE hDevice)
{
	base_address = 0;
	return BBM_OK;
}
