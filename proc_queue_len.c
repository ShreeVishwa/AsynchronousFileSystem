#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/sched.h>
#include<linux/uaccess.h>
#include<linux/fs.h>
#include<linux/seq_file.h>
#include<linux/slab.h>
#include "vars.h"

extern int queue_len = 0;

static int my_proc_show(struct seq_file *m,void *v){
	//seq_printf(m,"hello from proc file\n");
	seq_printf(m,"%d\n",queue_len);
	return 0;
}

static ssize_t my_proc_write(struct file* file,const char __user *buffer,size_t count,loff_t *f_pos){

	int val;
	int temp_val;

	val = kstrtoint_from_user(buffer,count,10,&temp_val);
	if(val){
		printk("Convert to int error code %d\n",val);
	}

	queue_len = temp_val;

	return count;
}

static int my_proc_open(struct inode *inode,struct file *file){
	return single_open(file,my_proc_show,NULL);
}

static struct file_operations my_fops={
	.owner = THIS_MODULE,
	.open = my_proc_open,
	.release = single_release,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = my_proc_write
};

static int __init hello_init(void){
	struct proc_dir_entry *entry;
	entry = proc_create("max_queue_len",0777,NULL,&my_fops);
	//printk("Entry is %d\n",entry);
	if(!entry){
		return -1;
	}else{
		printk(KERN_INFO "create max_len proc file successfully\n");
	}
	return 0;
}

static void __exit hello_exit(void){
	remove_proc_entry("max_queue_len",NULL);
	printk(KERN_INFO "Goodbye world!\n");
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
