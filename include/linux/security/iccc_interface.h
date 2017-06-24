/*
 * =====================================================================================
 *
 *       Filename:  Iccc_Interface.h
 *
 *    Description:  This header file must be in sync with other platform, tz related files
 *
 *        Version:  1.0
 *        Created:  08/20/2015 12:13:42 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Areef Basha (), areef.basha@samsung.com
 *        Company:  Samsung Electronics
 *
 *        Copyright (c) 2015 by Samsung Electronics, All rights reserved.
 *
 * =====================================================================================
 */
#ifndef Iccc_Interface_H_
#define Iccc_Interface_H_

#define	MAX_IMAGES      6
#define RESERVED_BYTES	96

/* ICCC section types are defined */
#define BL_ICCC_TYPE_START      0xFFF00000
#define TA_ICCC_TYPE_START      0xFF000000
#define KERN_ICCC_TYPE_START    0xFF100000
#define SYS_ICCC_TYPE_START     0xFF200000

#define ICCC_SECTION_MASK 0xfff00000
#define ICCC_SECTION_TYPE(type)		(ICCC_SECTION_MASK & (type))

/*  BL Secure Parameters */
#define RP_VER              (BL_ICCC_TYPE_START+0x00000)
#define KERNEL_RP           (BL_ICCC_TYPE_START+0x00001)
#define SYSTEM_RP           (BL_ICCC_TYPE_START+0x00002)
#define TEST_BIT            (BL_ICCC_TYPE_START+0x00003)
#define SEC_BOOT            (BL_ICCC_TYPE_START+0x00004)
#define REACT_LOCK          (BL_ICCC_TYPE_START+0x00005)
#define KIWI_LOCK           (BL_ICCC_TYPE_START+0x00006)
#define FRP_LOCK            (BL_ICCC_TYPE_START+0x00007)
#define CC_MODE             (BL_ICCC_TYPE_START+0x00008)
#define MDM_MODE            (BL_ICCC_TYPE_START+0x00009)
#define CURR_BIN_STATUS     (BL_ICCC_TYPE_START+0x0000A)
#define AFW_VALUE           (BL_ICCC_TYPE_START+0x0000B)
#define WARRANTY_BIT        (BL_ICCC_TYPE_START+0x0000C)
#define KAP_STATUS          (BL_ICCC_TYPE_START+0x0000D)
#define IMAGE_STATUS1       (BL_ICCC_TYPE_START+0x0000E)
#define IMAGE_STATUS2       (BL_ICCC_TYPE_START+0x0000F)
#define IMAGE_STATUS3       (BL_ICCC_TYPE_START+0x00010)
#define IMAGE_STATUS4       (BL_ICCC_TYPE_START+0x00011)
#define IMAGE_STATUS5       (BL_ICCC_TYPE_START+0x00012)
#define IMAGE_STATUS6       (BL_ICCC_TYPE_START+0x00013)
#define BL_STRUCT           (BL_ICCC_TYPE_START+0x000FF)

/* Kernel Secure Parameters */
#define PKM_TEXT        (TA_ICCC_TYPE_START+0x00000)
#define PKM_RO          (TA_ICCC_TYPE_START+0x00001)
#define SELINUX_STATUS  (TA_ICCC_TYPE_START+0x00002)

/* DMV Hash Parameter */
#define DMV_HASH		(KERN_ICCC_TYPE_START+0x00000)

/* System secure parameters */
#define SYSSCOPE_FLAG		(SYS_ICCC_TYPE_START+0x00000)
#define TRUSTBOOT_FLAG		(SYS_ICCC_TYPE_START+0x00001)

/* end Secure memory Parameters */

typedef enum iccc_error_code {
	ICCC_FAILURE = -1,
	ICCC_SUCCESS,
	ICCC_PERMISSION_DENIED,
} iccc_error_code_t;

uint32_t Iccc_SaveData_Kernel(uint32_t type, uint32_t value);
uint32_t Iccc_ReadData_Kernel(uint32_t type, uint32_t *value);

#endif /* Iccc_Interface_H_ */
