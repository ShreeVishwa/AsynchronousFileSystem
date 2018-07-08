#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/kernel.h>
#include<linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/namei.h>

struct file * open_file(char * const filepath, int acces, int offset)
{ 
         struct file *flip=NULL;
         mm_segment_t old_fs;
         int errno=0;

         old_fs = get_fs();
         set_fs(get_ds());
         flip = filp_open(filepath, acces, &flip->f_pos);
         set_fs(old_fs);
         if (IS_ERR(flip))
         {
                 errno = PTR_ERR(flip);
                 ferr=errno;
                 return NULL;
         }
         printk("\n************** %s file opened succesfully  *******************\n",filepath);
         return flip;

}

void close_file(struct file *flip,bool d)
{
        filp_close(flip, NULL);
        printk("\n************** file closed succesfully  *******************");
        return ;
}

int read_file(struct file *flip, unsigned  int len,unsigned char *data, unsigned long long  offset,bool d)//,void *buffer
{
        mm_segment_t old_fs;
        int return_data;      
        old_fs=get_fs();
        set_fs(get_ds());
        return_data=vfs_read(flip,data,len,&flip->f_pos);
        set_fs(old_fs);;
        printk("\n************** %s file read successfully *******************\n data read : %s\n\n ",flip->f_path.dentry->d_iname,data);
        return return_data;
}

int write_file(struct file *flip, unsigned  int len,unsigned char *data, unsigned long long  offset,bool d)
{
        mm_segment_t old_fs;
        int return_data = -1;
        flip->f_pos = 0;
        old_fs = get_fs();
        set_fs(get_ds());
        return_data = vfs_write( flip , data , len , &offset ) ;
        set_fs(old_fs);        
        printk("\n************** %s file was written successfully *******************\n data wirtten : %s\n\n ",flip->f_path.dentry->d_iname,data);
        return return_data;
}



