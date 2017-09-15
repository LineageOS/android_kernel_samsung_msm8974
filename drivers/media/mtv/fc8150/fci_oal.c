/*****************************************************************************
 Copyright(c) 2012 FCI Inc. All Rights Reserved

 File name : fci_oal.c

 Description : OS adaptation layer
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/mutex.h>

#include "fc8150_regs.h"
#include "fci_types.h"


static struct mutex isdbt_lock;


void PRINTF(HANDLE hDevice, char *fmt, ...)
{
	va_list ap;
	char str[256];

	va_start(ap, fmt);
	vsprintf(str, fmt, ap);

	printk(KERN_DEBUG"%s", str);

	va_end(ap);
}

void msWait(int ms)
{
	msleep(ms);
}

void OAL_CREATE_SEMAPHORE(void)
{
	mutex_init(&isdbt_lock);	
}

void OAL_DELETE_SEMAPHORE(void)
{
	mutex_destroy(&isdbt_lock);
}

void OAL_OBTAIN_SEMAPHORE(void)
{
	mutex_lock(&isdbt_lock);
}

void OAL_RELEASE_SEMAPHORE(void)
{
	mutex_unlock(&isdbt_lock);
}

