#include <linux/spi/spi.h>

#include "isdbt.h"
#include "isdbt_port_fc8300.h"

struct isdbt_drv_func fc8300_drv_func_struct;

extern void fc8300_set_port_if(unsigned long interface);
bool isfirst = true;

int probe_drv(void)
{
	int res;

	fc8300_set_port_if((unsigned long)isdbt_get_if_handle());
	
	res = isdbt_drv_probe();

	return res;
}

int remove_drv(void)
{
	int res;

	res = isdbt_drv_remove();
	isdbt_control_gpio(false);

	return res;
}

int hw_init_drv(unsigned long arg)
{
	if(isfirst)
	{
		isdbt_control_gpio(true);
		isdbt_control_gpio(false);
		isfirst = false;
	}

	isdbt_control_gpio(true);
//	isdbt_control_irq(true);
	isdbt_set_drv_mode(0);

	return arg;
}

void hw_deinit_drv(void)
{
	isdbt_control_gpio(false);
//	isdbt_control_irq(false);
}

int open_drv(struct inode *inode, struct file *filp)
{
	int res;

	res = isdbt_drv_open(inode, filp);

	return res;
}

ssize_t read_drv(struct file *filp
	, char *buf, size_t count, loff_t *f_pos)
{
	ssize_t size;

	size = isdbt_drv_read(filp, buf, count, f_pos);

	return size;
}

long ioctl_drv(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int res;

	res = isdbt_drv_ioctl(filp, cmd, arg);

	if (res == 0) {
		switch (cmd) {
			case IOCTL_ISDBT_POWER_ON:
				isdbt_power_on(0);
				break;
			case IOCTL_ISDBT_POWER_OFF:
				isdbt_power_off();
				break;
			default:
				break;
		}
	}
	return res;
}

int mmap_drv(struct file *filp, struct vm_area_struct *vma)
{
	return isdbt_drv_mmap(filp, vma);
}

irqreturn_t irq_drv(int irq, void *dev_id)
{
	return isdbt_threaded_irq(irq, dev_id);
}
int release_drv(struct inode *inode, struct file *filp)
{
	return isdbt_drv_release(inode, filp);
}

struct isdbt_drv_func *fc8300_drv_func(void)
{
	fc8300_drv_func_struct.probe = probe_drv;
	fc8300_drv_func_struct.remove = remove_drv;
	fc8300_drv_func_struct.power_on = hw_init_drv;
	fc8300_drv_func_struct.power_off = hw_deinit_drv;
	fc8300_drv_func_struct.open = open_drv;
	fc8300_drv_func_struct.read = read_drv;
	fc8300_drv_func_struct.ioctl = ioctl_drv;
	fc8300_drv_func_struct.mmap = mmap_drv;
	fc8300_drv_func_struct.irq_handler = irq_drv;
	fc8300_drv_func_struct.release = release_drv;

	return &fc8300_drv_func_struct;
}

