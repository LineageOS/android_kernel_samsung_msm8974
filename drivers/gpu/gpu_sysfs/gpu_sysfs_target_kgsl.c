#include	"gpu_sysfs_header.h"

/* *
 * *********************************************************************
 * Path defines for all the sysfs files.
 * *********************************************************************
 * */
#define		GPU_MIN_CLOCK		"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/devfreq/min_freq"
#define		GPU_MAX_CLOCK		"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/devfreq/max_freq"
#define		GPU_BUSY			"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/gpubusy"
#define		GPU_VOL				"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/vol"
#define		GPU_FREQ			"/sys/kernel/debug/clk/oxili_gfx3d_clk/measure"
#define		GPU_FREQ_TABLE		"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/gpu_available_frequencies"
#define		GPU_GOVERNOR		"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/devfreq/governor"
#define		GPU_AVAIL_GOVERNOR	"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/devfreq/available_governors"
/*#define	GPU_CORES_CONFIG	"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/core_mask" -- Not available  from sysfs. */
#define		GPU_TMU				"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/thermal_pwrlevel"
/*#define	GPU_MODEL			"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/modalias"  -- Not available  from sysfs. */
#define		GPU_VERSION			"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/uevent"
/*#define	GPU_MEM				"/sys/devices/fdb00000.qcom,kgsl-3d0/kgsl/kgsl-3d0/gpu_mem" -- Not available directly from sysfs. */
#define		QUALCOMM_SYSFS_GPU_FPS		"/sys/devices/platform/gpusysfs/fps"
/*
 * atoi function ----
 * Taken from drivers/video/msm/external_common.c
 * from Kernel source for KLTE device.
 * */
static int atoi_ignore_space(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
		case '0' ... '9':
			val = 10*val+(*name-'0');
			break;
		case ' ':
			break;
		default:
			return val;
		}
	}
}

/* *
 * *********************************************************************
 * Device ATTR functions. Will be called when read from sysfs.
 * *********************************************************************
 * */
ssize_t gpu_min_clock_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_32];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_MIN_CLOCK, input_buffer, INPUT_BUFFER_SIZE_32);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_max_clock_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_32];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_MAX_CLOCK, input_buffer, INPUT_BUFFER_SIZE_32);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_busy_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char		time_consumed_str[INPUT_BUFFER_SIZE_32] = {0};
	char		time_elapsed_str[INPUT_BUFFER_SIZE_32] = {0};
	unsigned int	time_consumed = 0;
	unsigned int	time_elapsed = 0;
	char			input_buffer[INPUT_BUFFER_SIZE_32];
	int				status = 0;

	status = open_file_and_read_buffer(GPU_MAX_CLOCK, input_buffer, INPUT_BUFFER_SIZE_32);

	if (status != SRUK_TRUE)
	{
		return sprintf(buf, "-1");
	}

	/* ************************************ */
	/* Parse input to find utilization ratio.
	 * This is target specific.
	 * kgsl drive outputs utilzation in terms of
	 * elapsed time using "%7d %7d"
	 * */
	/* ************************************ */
	strncpy(time_consumed_str, input_buffer,   7);
	strncpy(time_elapsed_str,  input_buffer+8, 7);

	time_consumed = atoi_ignore_space(time_consumed_str);
	time_elapsed = atoi_ignore_space(time_elapsed_str);

	return sprintf(buf, "%d\n", ((time_consumed*100)/time_elapsed));
}

ssize_t gpu_vol_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "gpu_voltage      --       Not available.\n");
}

ssize_t gpu_freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_32];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_FREQ, input_buffer, INPUT_BUFFER_SIZE_32);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_freq_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
	if (open_file_and_write_buffer(GPU_GOVERNOR, "performance", strlen("performance")) == 0)
	{
		pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
		return 0;
	}
/*
	pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
	if (open_file_and_write_buffer(GPU_FREQ, buf, strlen(buf)) == 0)
	{
		pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
		return 0;
	}
*/
	pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
	/* Return success status. */
	return count;
}

ssize_t gpu_freq_table_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_128];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_FREQ_TABLE, input_buffer, INPUT_BUFFER_SIZE_128);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_governor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_32];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_GOVERNOR, input_buffer, INPUT_BUFFER_SIZE_32);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_governor_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
	if (open_file_and_write_buffer(GPU_GOVERNOR, buf, strlen(buf)) == 0)
	{
		pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
		return 0;
	}

	pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);

	/* Return success status. */
	return count;
}

ssize_t gpu_available_governor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_256];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_AVAIL_GOVERNOR, input_buffer, INPUT_BUFFER_SIZE_256);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_cores_config_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "gpu_core_config  --       Not available.\n");
}

ssize_t gpu_tmu_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char	input_buffer[INPUT_BUFFER_SIZE_32];
	int		status = 0;

	status = open_file_and_read_buffer(GPU_TMU, input_buffer, INPUT_BUFFER_SIZE_32);

	if (status == SRUK_TRUE)
	{
		return sprintf(buf, "%s", input_buffer);
	}
	else
	{
		return sprintf(buf, "-1");
	}
}

ssize_t gpu_model_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char			input_buffer[INPUT_BUFFER_SIZE_128];
	int				status = 0;
	char			*parse_pointer = input_buffer;
	int			char_counter = 0, equal_char_counter = 0;

	status = open_file_and_read_buffer(GPU_VERSION, input_buffer, INPUT_BUFFER_SIZE_128);

	if (status != SRUK_TRUE)
	{
		return sprintf(buf, "-1");
	}

	/* ************************************ */
	/* Parse input to find gpu version.
	 * This is target specific.
	 * The driver gives information in following
	 * format:
	 *    MAJOR=242
	 *    MINOR=0
	 *    DEVNAME=kgsl-3d0
	 * */
	/* ************************************ */

	while (*parse_pointer != '\0')
	{
		/* Look for '='. */
		if (*parse_pointer == '=')
		{
			equal_char_counter++;
		}

		/* */
		parse_pointer++;
		char_counter++;

		if (equal_char_counter == 3)
		{
			break;
		}
	}

	/* Copy the model string to the output string.*/
	return sprintf(buf, "%s\n", input_buffer+char_counter);
}

ssize_t gpu_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char		version_major_str[INPUT_BUFFER_SIZE_16] = {0};
	char		version_minor_str[INPUT_BUFFER_SIZE_16] = {0};
	unsigned int	version_major = 0, version_minor = 0;
	char			input_buffer[INPUT_BUFFER_SIZE_128];
	int				status = 0;

	status = open_file_and_read_buffer(GPU_VERSION, input_buffer, INPUT_BUFFER_SIZE_128);

	if (status != SRUK_TRUE)
	{
		return sprintf(buf, "-1");
	}

	/* ************************************ */
	/* Parse input to find gpu version.
	 * This is target specific.
	 * The driver gives information in following
	 * format:
	 *    MAJOR=242
	 *    MINOR=0
	 *    DEVNAME=kgsl-3d0
	 * */
	/* ************************************ */

	strncpy(version_major_str, input_buffer+6, 5);
	strncpy(version_minor_str, input_buffer+17, 5);

	version_major = atoi_ignore_space(version_major_str);
	version_minor = atoi_ignore_space(version_minor_str);

	return sprintf(buf, "%d.%d\n", version_major, version_minor);
}

ssize_t gpu_mem_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "gpu_mem -- Not available.\n");
}

char	 global_fps_string[INPUT_BUFFER_SIZE_32];
ssize_t fps_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s", global_fps_string);
}

ssize_t fps_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
    if (buf != NULL)
	{
        snprintf(global_fps_string,sizeof(global_fps_string),"%s", buf);
	}
    else
	{
        sprintf(global_fps_string,"0");
	}

	/* Return success status. */
	return count;

}
