/*
 * Copyright 2011 Validity Sensors, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 */

#ifndef VFS61XX_H_
#define VFS61XX_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <asm-generic/uaccess.h>
#include <linux/irq.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/wakelock.h>
#include <linux/fprint_secure.h>

/* Major number of device ID.
 * A device ID consists of two parts: a major number, identifying the class of
 * the device, and a minor ID, identifying a specific instance of a device in
 * that class. A device ID is represented using the type dev_t. The minor number
 * of the Validity device is 0. */
#define VFSSPI_MAJOR         (221)

/* Maximum transfer size */
#define DEFAULT_BUFFER_SIZE  (4096 * 5)

#define DRDY_ACTIVE_STATUS      0
#define BITS_PER_WORD           16
#define DRDY_IRQ_FLAG           IRQF_TRIGGER_FALLING
#define VENDOR		"SYNAPTICS"
#define CHIP_ID		"RAPTOR"

/* Timeout value for polling DRDY signal assertion */
#define DRDY_TIMEOUT_MS      40

/*
 * Definitions of structures which are used by IOCTL commands
 */

#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Pass to VFSSPI_IOCTL_SET_USER_DATA
 * and VFSSPI_IOCTL_GET_USER_DATA commands */
struct vfsspi_iocUserData {
	void *buffer;
	unsigned int len;
};
#endif

#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Pass to VFSSPI_IOCTL_RW_SPI_MESSAGE command */
struct vfsspi_iocTransfer {
	unsigned char *rxBuffer;	/* pointer to retrieved data */
	unsigned char *txBuffer;	/* pointer to transmitted data */
	unsigned int len;	/* transmitted/retrieved data size */
};
#endif

/* Pass to VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL command */
struct vfsspi_iocRegSignal {
	/* Process ID to which SPI driver sends signal
	 * indicating that DRDY is asserted */
	int userPID;
	int signalID;		/* Signal number */
};

#ifdef CONFIG_SENSORS_FINGERPRINT_SYSFS
extern int fingerprint_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
extern void fingerprint_unregister(struct device *dev,
	struct device_attribute *attributes[]);
#endif

/* Magic number of IOCTL command */
#define VFSSPI_IOCTL_MAGIC    'k'

/*
 * IOCTL commands definitions
 */

#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Transmit data to the device
and retrieve data from it simultaneously */
#define VFSSPI_IOCTL_RW_SPI_MESSAGE	\
	_IOWR(VFSSPI_IOCTL_MAGIC, 1, unsigned int)
#endif
/* Hard reset the device */
#define VFSSPI_IOCTL_DEVICE_RESET	\
	_IO(VFSSPI_IOCTL_MAGIC,   2)
/* Set the baud rate of SPI master clock */
#define VFSSPI_IOCTL_SET_CLK	\
	_IOW(VFSSPI_IOCTL_MAGIC,  3, unsigned int)
#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Get level state of DRDY GPIO */
#define VFSSPI_IOCTL_CHECK_DRDY	\
	_IO(VFSSPI_IOCTL_MAGIC,   4)
#endif
/* Register DRDY signal. It is used by SPI driver
 * for indicating host that DRDY signal is asserted. */
#define VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL	\
	_IOW(VFSSPI_IOCTL_MAGIC,  5, unsigned int)
/* Store the user data into the SPI driver. Currently user data is a
 * device info data, which is obtained from announce packet. */
#ifndef ENABLE_SENSORS_FPRINT_SECURE
#define VFSSPI_IOCTL_SET_USER_DATA	\
	_IOW(VFSSPI_IOCTL_MAGIC,  6, unsigned int)
#endif
#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Retrieve user data from the SPI driver*/
#define VFSSPI_IOCTL_GET_USER_DATA	\
	_IOWR(VFSSPI_IOCTL_MAGIC, 7, unsigned int)
#endif
/* Enable/disable DRDY interrupt handling in the SPI driver */
#define VFSSPI_IOCTL_SET_DRDY_INT	\
	_IOW(VFSSPI_IOCTL_MAGIC,  8, unsigned int)
/* Put device in Low power mode */
#define VFSSPI_IOCTL_DEVICE_SUSPEND	\
	_IO(VFSSPI_IOCTL_MAGIC,	9)
#ifndef ENABLE_SENSORS_FPRINT_SECURE
/* Indicate the fingerprint buffer size for read */
#define VFSSPI_IOCTL_STREAM_READ_START	\
	_IOW(VFSSPI_IOCTL_MAGIC, 10, unsigned int)
/* Indicate that fingerprint acquisition is completed */
#define VFSSPI_IOCTL_STREAM_READ_STOP	\
	_IO(VFSSPI_IOCTL_MAGIC,   11)
/* Retrieve supported SPI baud rate table */
#define VFSSPI_IOCTL_GET_FREQ_TABLE	\
	_IOWR(VFSSPI_IOCTL_MAGIC, 12, unsigned int)
#endif
/* Turn on the power to the sensor */
#define VFSSPI_IOCTL_POWER_ON	\
	_IO(VFSSPI_IOCTL_MAGIC,   13)
/* Turn off the power to the sensor */
#define VFSSPI_IOCTL_POWER_OFF	\
	_IO(VFSSPI_IOCTL_MAGIC,   14)
#ifdef ENABLE_SENSORS_FPRINT_SECURE
/* To disable spi core clock */
#define VFSSPI_IOCTL_DISABLE_SPI_CLOCK	\
	_IO(VFSSPI_IOCTL_MAGIC, 15)
/* To set SPI configurations like gpio, clks */
#define VFSSPI_IOCTL_SET_SPI_CONFIGURATION	\
	_IO(VFSSPI_IOCTL_MAGIC, 16)
/* To reset SPI configurations */
#define VFSSPI_IOCTL_RESET_SPI_CONFIGURATION	\
	_IO(VFSSPI_IOCTL_MAGIC, 17)
#endif
/* get sensor orienation from the SPI driver*/
#define VFSSPI_IOCTL_GET_SENSOR_ORIENT	\
	_IOR(VFSSPI_IOCTL_MAGIC, 18, unsigned int)
#endif /* VFS61XX_H_ */
