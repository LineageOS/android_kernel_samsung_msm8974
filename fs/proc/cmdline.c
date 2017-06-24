#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/module.h>

static int cmdline_proc_show(struct seq_file *m, void *v)
{
	char *temp_saved_command_line, *temp;
	int i;
	
	temp_saved_command_line = kmalloc(strlen(saved_command_line)+1, GFP_KERNEL);
	memcpy(temp_saved_command_line, saved_command_line, strlen(saved_command_line)+1);
	temp=strstr(temp_saved_command_line, "array");

	if (temp!=NULL) {
		for(i=0;i<20;i++)	*(temp+i)='*';
	}

	seq_printf(m, "%s\n", temp_saved_command_line);

	kfree(temp_saved_command_line);
	return 0;
}

static int cmdline_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmdline_proc_show, NULL);
}

static const struct file_operations cmdline_proc_fops = {
	.open		= cmdline_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_cmdline_init(void)
{
	proc_create("cmdline", 0, NULL, &cmdline_proc_fops);
	return 0;
}
module_init(proc_cmdline_init);
