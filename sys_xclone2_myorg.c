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
#include <linux/crypto.h>

asmlinkage extern long (*sysptr)(void *arg);

struct myargs {
	const char *file;
	u_int flags;
};
int decompress_buffer(char * in_buf, int in_len,char *out_buf,int out_len);

int compress_buffer(char * in_buf, int in_len,char *out_buf, int out_len)//, int *compressor_type )
{
	int err;//o_len;
	// int out_len1 = kmalloc( sizeof(int) , GFP_KERNEL);
	const char *cmp_alg = "deflate";
	struct crypto_comp *tfm;
	// char * out_buf1 = (char *)kmalloc( sizeof(char)*PAGE_SIZE , GFP_KERNEL);
	tfm = crypto_alloc_comp(cmp_alg,0,0);
    err = crypto_comp_compress(tfm, in_buf, in_len, out_buf,&out_len);
    if (err < 0)
    {
    	printk("\nERROR : Failure in compression");
    	return -1;
    }
    // o_len = sizeof(out_buf);
    // printk("\nFHKJN : %d %s %d %s\n",out_len, out_buf, o_len, in_buf);
    // err = decompress_buffer(out_buf,out_len,out_buf1,out_len1);
    // printk("\n ______the decompressed data lenght is : %d ",err);
    // kfree(out_buf1);
    printk("\n ______the compressed data is : %s ",out_buf);
    return out_len;    
}

int decompress_buffer(char * in_buf, int in_len,char *out_buf, int out_len)//, int *compressor_type )
{
	int err;//o_len;
	const char *cmp_alg = "deflate";
	struct crypto_comp *tfm;
	// char * out_buf1 = (char *)kmalloc( sizeof(in_buf) , GFP_KERNEL);
	tfm = crypto_alloc_comp(cmp_alg,0,0);
    err = crypto_comp_decompress(tfm, in_buf, in_len, out_buf,&out_len);
    if (err < 0)
    {
    	printk("\nERROR : Failure in decompression : %d ", err);
    	return -1;
    }
    printk("\n ***** the decompressed data is : %s ",out_buf);
    return out_len;
}

int compress_file(struct file * f_read, struct file * f_write)
{
	int bytes1 , bytes2 , err = 0 , src_len , curr_read_size = 0 , curr_write_size = 0, outlen;
	int fsize , temp_size, te;
	char *buff3 , *buff4, tei[4];
	mm_segment_t oldfs1;
	mm_segment_t oldfs2;
	int out_len [1];

	struct inode *Inode;

	Inode = f_read->f_path.dentry->d_inode;
    fsize = i_size_read(Inode);
 	while(fsize > 0){

                buff3 = (char*)kmalloc(sizeof(char)*PAGE_SIZE,GFP_KERNEL);
                buff4 = (char*)kmalloc(sizeof(char)*PAGE_SIZE,GFP_KERNEL);
                if(fsize > PAGE_SIZE - 16){
                    temp_size = PAGE_SIZE - 16;
                    fsize = fsize - temp_size;
                }
                else {
                    temp_size = fsize;
                    fsize = 0;
                }
                f_read->f_pos = curr_read_size;
                oldfs1 = get_fs();
                set_fs(KERNEL_DS);
                bytes1 = vfs_read(f_read, buff3,temp_size, &f_read->f_pos);
                set_fs(oldfs1);
                printk("data is %s\n",buff3);
                printk("Bytes read is %d\n",bytes1);

                printk("About to encrypt the file\n");
                src_len = bytes1;
                err = compress_buffer(buff3,bytes1,buff4,out_len[0]);
                printk("\n_______length : %d\n",err);
                if(err < 0){
                    printk("Compression failed\n");
                    kfree(buff3);
                    kfree(buff4);
                   
                    return err;
                }
                outlen = err;
                printk("Compression_success\n");
                printk("Compressed buff is %s\n",buff4);

                f_write->f_pos = curr_write_size;
                oldfs2 = get_fs();
                set_fs(KERNEL_DS);
               
                te = snprintf( tei,4,"%d",outlen);
                printk("\n ______ the psize : %s \n",tei);
                bytes2 = vfs_write(f_write, tei,4, &f_write->f_pos);
                
                bytes2 = vfs_write(f_write, buff4, sizeof(buff4), &f_write->f_pos);
                
                set_fs(oldfs2);
                printk("%d\n",bytes2);

                printk("Successfully written the file\n");
                kfree(buff3);
                kfree(buff4);
                printk("successfully freed the buffer\n");

                curr_read_size += bytes1;
                curr_write_size += bytes2;

            }
      return err;
}
int decompress_file(struct file * f_read, struct file * f_write)
{
	int bytes1 , bytes2 , err = 0 , src_len , curr_read_size = 0 , curr_write_size = 0, d_len;
	int fsize , temp_size, psize,te;
	char *buff3 , *buff4, ps[4];
	mm_segment_t oldfs1;
	mm_segment_t oldfs2;

	struct inode *Inode;
	int out_len [1];//= (int)kmalloc( sizeof(int) , GFP_KERNEL);

	Inode = f_read->f_path.dentry->d_inode;
    fsize = i_size_read(Inode);
 	while(fsize > 0){
 		        //psize = (int*)kmalloc(sizeof(char)*4,GFP_KERNEL);


                buff3 = (char*)kmalloc(sizeof(char)*PAGE_SIZE,GFP_KERNEL);
                buff4 = (char*)kmalloc(sizeof(char)*PAGE_SIZE,GFP_KERNEL);
                if(fsize > PAGE_SIZE - 16){
                    temp_size = PAGE_SIZE - 16;
                    fsize = fsize - temp_size;
                }
                else {
                    temp_size = fsize;
                    fsize = 0;
                }
                f_read->f_pos = curr_read_size;
                oldfs1 = get_fs();
                set_fs(KERNEL_DS);
                bytes1 = vfs_read(f_read, ps,4, &f_read->f_pos);
                te = kstrtoint(ps , 10, &psize);
                bytes1 = vfs_read(f_read, buff3,psize, &f_read->f_pos);
                set_fs(oldfs1);
                printk("data is %s\n",buff3);
                printk("Bytes read is %d\n",bytes1);

                printk("About to decompress the file\n");
                src_len = bytes1;
                d_len = decompress_buffer(buff3,psize,buff4,out_len[1]);
                if(d_len < 0){
                    printk("Decompression failed\n");
                    kfree(buff3);
                    kfree(buff4);
                    return d_len;
                }
                printk("Decompression_success\n");
                printk("Decompressed buff is %s\n",buff4);

                f_write->f_pos = curr_write_size;
                oldfs2 = get_fs();
                set_fs(KERNEL_DS);
                bytes2 = vfs_write(f_write, buff4, d_len, &f_write->f_pos);
                set_fs(oldfs2);
                printk("%d\n",bytes2);

                printk("Successfully written the file\n");
                kfree(buff3);
                kfree(buff4);
                printk("successfully freed the buffer\n");

                curr_read_size += bytes1;
                curr_write_size += bytes2;

            }
      return err;
}
struct file * open_file(char * filepath, int acces, int offset)
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
                 // err=errno;
                 return NULL;
         }
         printk("\n************** %s file opened succesfully  *******************\n",filepath);
         return flip;

}
void close_file(struct file *flip)
{
        filp_close(flip, NULL);
        printk("\n************** file closed succesfully  *******************");
        return ;
}


asmlinkage
long xclone2(void *arg) {
	struct myargs *args;
	int err = 0;
	// char *out_buf = (char *)kmalloc( 15 , GFP_KERNEL);
	// int out_len = kmalloc( sizeof(int) , GFP_KERNEL);
	struct file *in_file , *out_file , *out_file1;


	printk("inside of clone2\n");
	
	if (arg == NULL)
		return -EINVAL;
	else {

		args = (struct myargs *) arg;
		if (!args->file) {
			printk("Infile1 is NULL");
			err = -EINVAL;
			goto out;
		}

		printk("recieved  filename  %s\n", args->file);
	}
	in_file = open_file( args -> file, O_RDONLY, 0);
    out_file = open_file( "out.txt", O_CREAT|O_WRONLY, 0);
    out_file1 = open_file( "out.txt.cmp", O_CREAT|O_WRONLY, 0);

    err = compress_file(in_file,out_file);
    close_file(in_file);
    close_file(out_file);
    out_file = open_file( "out.txt", O_RDONLY, 0);

    err = decompress_file(out_file,out_file1);
    close_file(out_file);
    close_file(out_file1);
	// err = compress_buffer("AAAABBBBCC",10,out_buf);
    printk("\nerr of compression  : %d\n",err);

	out: 
	
     // kfree(out_buf);
     // kfree(out_len);
	return err;
}

static int __init
init_sys_xclone2(void) {
	printk("installed new sys_xclone2 module\n");
	if (sysptr == NULL)
		sysptr = xclone2;
	return 0;
}
static void __exit
exit_sys_xclone2(void) {
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_xclone2 module\n");
}
module_init( init_sys_xclone2);
module_exit( exit_sys_xclone2);
MODULE_LICENSE("GPL");
