#ifndef		__GPU_SYSFS_HEADER_H__
#define		__GPU_SYSFS_HEADER_H__

/* Linux kernel header. */
#include	<linux/kobject.h>
#include	<linux/string.h>
#include	<linux/sysfs.h>
#include	<linux/module.h>
#include	<linux/init.h>
#include	<linux/platform_device.h>
#include	<linux/slab.h>

/* File handling related headers. */
#include	<linux/fs.h>
#include	<asm/segment.h>
#include	<asm/uaccess.h>
#include	<linux/buffer_head.h>
#include	<linux/string.h>

/* Some necessary defines and typedefs. */
#define		GPU_SYSFS_MODULE_NAME	"gpusysfs"
#define		DEVNAME_SIZE			32
#define		SRUK_FALSE			1
#define		SRUK_TRUE			0
#define		SRUK_DRV_NAME			"sruk"
typedef		unsigned int		sruk_bool;
static const char				sruk_drv_name[] = SRUK_DRV_NAME;

/* Device and attribute data structure. */
typedef struct sruk_os_device
{
	struct list_head	entry;
	struct device	   *dev;
	char				devname[DEVNAME_SIZE];
} sruk_os_device;

typedef struct sruk_attribute
{
	int id;
	uintptr_t data;
} sruk_attribute;

typedef struct sruk_device
{
	sruk_os_device		osdev;
} sruk_device;

/* *
 * *********************************************************************
 * Function prototypes for sysfs.
 * *********************************************************************
 * */
ssize_t gpu_min_clock_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_max_clock_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_busy_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_vol_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_freq_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_freq_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t gpu_freq_table_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_governor_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_governor_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
ssize_t gpu_available_governor_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_cores_config_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_tmu_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_model_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_version_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t gpu_mem_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t fps_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t fps_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

/* *
 * *********************************************************************
 * File handling related function prototypes..
 * *********************************************************************
 * */
struct file* file_open(const char* path, int flags, int rights) ;
void	file_close(struct file *file_ptr);
int	file_read(struct file *file_ptr, unsigned long long offset, unsigned char* data, unsigned int size);
int	file_write(struct file *file_ptr, unsigned long long offset, unsigned char* data, unsigned int size);
int	file_sync(struct file* file_ptr);
int	open_file_and_read_buffer(char *filename_and_path, char *input_buffer, int input_buffer_size);
int	open_file_and_write_buffer(char *filename_and_path, const char *buffer, int buffer_size);

/* *
 * *********************************************************************
 * Buffer size for reading from sysfs files.
 * *********************************************************************
 * */
#define		INPUT_BUFFER_SIZE_16	16
#define		INPUT_BUFFER_SIZE_32	32
#define		INPUT_BUFFER_SIZE_64	64
#define		INPUT_BUFFER_SIZE_128	128
#define		INPUT_BUFFER_SIZE_256	256
#define		INPUT_BUFFER_SIZE_512	512

#endif		/* __GPU_SYSFS_HEADER_H__ */
