#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>

//#include <plat/gpio-cfg.h>

//#include <mach/msm8930-gpio.h>

extern struct class *sec_class;
struct device *slot_switch_dev;

struct slot_switch_wq {
	struct delayed_work work_q;
//	struct fsa9480_info *sdata;
	struct list_head entry;
};

#ifdef CONFIG_MACH_H3GDUOS_CTC
#define GPIO_SIM_SEL 142
#define GPIO_GG_SEL 115
#else
#define GPIO_SIM_SEL 123
#define GPIO_GG_SEL 0
#endif

static ssize_t get_slot_switch(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value;

	//return '0' slot path is '||', return '1' slot path is 'X'
	value = gpio_get_value(GPIO_SIM_SEL);
	printk("Current Slot is %x\n", value);

	return sprintf(buf, "%d\n", value);
}

// 1 : BCOM
// 0 : SPRD
static ssize_t set_slot_switch(struct device *dev, struct device_attribute *attr,   const char *buf, size_t size)
{
	int value;
	int fd;
	char buffer[2]={0};
	mm_segment_t fs;

	sscanf(buf, "%d", &value);

	fs = get_fs();
	set_fs(get_ds());

	if ((fd = sys_open("/efs/slot_witch.bin", O_CREAT|O_WRONLY, 0)) < 0)
	{
		printk("%s :: open failed %s ,fd=0x%x\n", __func__, "/efs/slot_witch.bin", fd);
	} else {
		printk("%s :: open success %s ,fd=0x%x\n", __func__, "/efs/slot_witch.bin", fd);
	}

	printk("%s : value = %d \n", __func__,value);

	switch(value) {
		case 0:
			gpio_set_value(GPIO_SIM_SEL, 0);
			sprintf(buffer, "%s", "0");
			printk("set slot switch to %x\n", gpio_get_value(GPIO_SIM_SEL));
			break;
		case 1:
			gpio_set_value(GPIO_SIM_SEL, 1);
			sprintf(buffer, "1");			
			printk("set slot switch to %x\n", gpio_get_value(GPIO_SIM_SEL));
			break;
		default:
			printk("Enter 0 or 1!!\n");
	}

	sys_write(fd, buffer, strlen(buffer));

	sys_close(fd); 
	set_fs(fs);

	return size;
}

static DEVICE_ATTR(slot_sel, S_IRUGO | S_IWUSR | S_IWGRP, get_slot_switch, set_slot_switch);

static void slot_switch_init_work(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct slot_switch_wq *wq = container_of(dw, struct slot_switch_wq, work_q);
//	struct fsa9480_info *usbsw = wq->sdata;
	int fd;
	int ret;
	char buffer[2]={0};
	mm_segment_t fs;

	printk("[slot switch]: %s :: \n",__func__);

	fs = get_fs();
	set_fs(get_ds());

	if ((fd = sys_open("/efs/slot_switch.bin", O_CREAT|O_RDWR  ,0664)) < 0)
	{
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(2000));
		printk("[slot switch]: %s :: open failed %s ,fd=0x%x\n",__func__,"/efs/slot_switch.bin",fd);

		if(fd < 0){
			sys_close(fd);
			set_fs(fs);

		}
	} else {
		cancel_delayed_work(&wq->work_q);
		printk("[slot switch]: %s :: open success %s ,fd=0x%x\n",__func__,"/efs/slot_switch.bin",fd);
	}

	ret = sys_read(fd, buffer, 1);
	if(ret < 0) {
		printk("slot_switch READ FAIL!\n");
		sys_close(fd);
		set_fs(fs);
		return;
	}

	printk("slot switch buffer : %s\n", buffer);
#if 0//!defined(CONFIG_MACH_MELIUS_CHN_CTC) && !defined(CONFIG_MACH_CRATER_CHN_CTC)
	if (!strcmp(buffer, "0"))//SPRD
	{
		gpio_set_value(GPIO_SIM_SEL, 0);
		printk("set slot switch to %x\n", gpio_get_value(GPIO_SIM_SEL));
	} else if(!strcmp(buffer, "1")){//BCOM
		gpio_set_value(GPIO_SIM_SEL, 1);
		printk("set slot switch to %x\n", gpio_get_value(GPIO_SIM_SEL));
	}
#endif
	sys_close(fd);
	set_fs(fs);

	return;
}

static int __init slot_switch_manager_init(void)
{
	int ret = 0;
	int err = 0;
	int gpio = 0;
	struct slot_switch_wq *wq;

	printk("slot_switch_manager_init\n");

    //initailize uim_sim_switch gpio
	printk("%s start \n",__func__);
	gpio = GPIO_SIM_SEL;
	err = gpio_request(gpio, "SIM_SEL");
	if (err) {
		pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"PDA_ACTIVE", GPIO_SIM_SEL, err);
	} else {
		gpio_tlmm_config(GPIO_CFG(GPIO_SIM_SEL,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_direction_output(gpio, 0);
		gpio_export(gpio, 0);
		gpio_set_value(GPIO_SIM_SEL, 0);
		printk("%s end \n",__func__);
	}

#ifdef CONFIG_MACH_H3GDUOS_CTC
	gpio = GPIO_GG_SEL;
	err = gpio_request(gpio, "GG_SEL");
	if (err) {
		pr_err("fail to request gpio %s, gpio %d, errno %d\n",
					"PDA_ACTIVE", GPIO_GG_SEL, err);
	} else {
		gpio_tlmm_config(GPIO_CFG(GPIO_GG_SEL,  0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_direction_output(gpio, 0);
		gpio_export(gpio, 0);
		gpio_set_value(GPIO_GG_SEL, 1);  // fixed to '1' from '0' by jw01.yoo
		printk("%s end \n",__func__);
	}
#endif

	//initailize slot switch device
	slot_switch_dev = device_create(sec_class,
                                    NULL, 0, NULL, "slot_switch");
	if (IS_ERR(slot_switch_dev))
		pr_err("Failed to create device(switch)!\n");

	if (device_create_file(slot_switch_dev, &dev_attr_slot_sel) < 0)
		pr_err("Failed to create device file(%s)!\n",
					dev_attr_slot_sel.attr.name);

	wq = kmalloc(sizeof(struct slot_switch_wq), GFP_ATOMIC);
	if (wq) {
//		wq->sdata = usbsw;
		INIT_DELAYED_WORK(&wq->work_q, slot_switch_init_work);
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(100));
	} else
		return -ENOMEM;

	return ret;
}

static void __exit slot_switch_manager_exit(void)
{
}

module_init(slot_switch_manager_init);
module_exit(slot_switch_manager_exit);

MODULE_AUTHOR("SAMSUNG ELECTRONICS CO., LTD");
MODULE_DESCRIPTION("Slot Switch");
MODULE_LICENSE("GPL");
