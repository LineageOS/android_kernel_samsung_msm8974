#include	"gpu_sysfs_header.h"

/* *
 * *********************************************************************
 * File handling related functions.
 * *********************************************************************
 * */
struct file* file_open(const char* path, int flags, int rights)
{
    struct file* input_file_ptr = NULL;
    mm_segment_t old_filesystem;
    int err = 0;

    old_filesystem = get_fs();
    set_fs(get_ds());
    input_file_ptr = filp_open(path, flags, rights);
    set_fs(old_filesystem);
    if(IS_ERR(input_file_ptr))
    {
        err = PTR_ERR(input_file_ptr);
        return NULL;
    }
    return input_file_ptr;
}

void file_close(struct file *file_ptr)
{
    filp_close(file_ptr, NULL);
}

int file_read(struct file *file_ptr, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t old_filesystem;
    int ret;

    old_filesystem = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file_ptr, data, size, &offset);

    set_fs(old_filesystem);
    return ret;
}

int file_write(struct file *file_ptr, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t old_filesystem;
    int ret;

    old_filesystem = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file_ptr, data, size, &offset);

    set_fs(old_filesystem);
    return ret;
}

int file_sync(struct file* file_ptr)
{
    vfs_fsync(file_ptr, 0);
    return 0;
}

int open_file_and_read_buffer(char *filename_and_path, char *input_buffer, int input_buffer_size)
{
	struct file* input_file_ptr;

	input_file_ptr = file_open(filename_and_path, O_RDONLY, 0);
	if (input_file_ptr != NULL)
	{
		int ret_val = file_read(input_file_ptr, 0, input_buffer, input_buffer_size);
		if ((ret_val >= 0) && (ret_val < input_buffer_size))
		    input_buffer[ret_val] = '\0';
		else if (ret_val == input_buffer_size)
			input_buffer[ret_val-1] = '\0';
		file_close(input_file_ptr);

		return SRUK_TRUE;
	}

	return SRUK_FALSE;
}

/* The function returns the number of bytes written to the file. */
int open_file_and_write_buffer(char *filename_and_path, const char *buffer, int buffer_size)
{
	struct file* input_file_ptr;
	int ret_val = 0;

    pr_info("SRUK ----------- %s -- %d", __FUNCTION__, __LINE__);
	input_file_ptr = file_open(filename_and_path, O_WRONLY, 0);
	if (input_file_ptr != NULL)
	{
		pr_info("SRUK ----------- %s -- %d -- %s -- %d", __FUNCTION__, __LINE__, buffer, buffer_size);
		ret_val = file_write(input_file_ptr, 0, (unsigned char *)buffer, buffer_size);
		file_sync(input_file_ptr);
		file_close(input_file_ptr);
	}

    pr_info("SRUK ----------- %s -- %d -- ret_val = %d ", __FUNCTION__, __LINE__ , ret_val);
	return ret_val;
}
