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
#include <linux/sched.h>
#include <asm/segment.h>
#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/uidgid.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/key-type.h>
#include <linux/ceph/decode.h>
#include <crypto/md5.h>
#include <crypto/aes.h>
#include <crypto/hash.h>
#include <keys/ceph-type.h>
#include <linux/hash.h>
#include <linux/ceph/types.h>
#include <linux/ceph/buffer.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/errno.h>
#include <linux/dcache.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <crypto/skcipher.h>
#include <linux/limits.h>
#include <linux/time.h>
#include <linux/xattr.h>
#include <linux/rtc.h>
#include "params.h"
#include "flags.h"
#include "sys_xclone2async.h"
extern char* enc_key;

asmlinkage extern long (*sysptr)(void *arg);

static int create_trashbin(void)
{
	int err=0;
	char *trashbin_folder_path;
	struct dentry *tbdentry;
	struct path path;



	trashbin_folder_path = kmalloc(strlen("/usr")+sizeof(char)*9 ,GFP_KERNEL);

	if (!trashbin_folder_path)
	{
		printk("error creating in trash folder name \n");
		err = PTR_ERR(trashbin_folder_path);
		goto out;
	}
	else
	{
		memset(trashbin_folder_path, '\0', strlen("/usr")+sizeof(char)*9 );
		memcpy(trashbin_folder_path, "/usr", strlen("/usr"));
		strcat(trashbin_folder_path, "/trashbin");
		printk("trash folder name %s\n",trashbin_folder_path);
	}


	/* check for dentry */
	tbdentry = kern_path_create(AT_FDCWD, trashbin_folder_path, &path, LOOKUP_DIRECTORY);

	if (!tbdentry || IS_ERR(tbdentry))
	{
		err=-PTR_ERR(tbdentry);
		printk("trash bin folder already present in the directory\n");
	}
	else
	{
		done_path_create(&path, tbdentry);
		inode_lock_nested(d_inode(path.dentry), I_MUTEX_PARENT);
		err = vfs_mkdir(d_inode(path.dentry), tbdentry,d_inode(path.dentry)->i_mode);
	    	inode_unlock(d_inode(path.dentry));
     	//err = create_path(trashbin_folder_path);

		if(err<0)
		{
			printk("failed to create trashbin folder in the directory\n");
			kfree(trashbin_folder_path);
			goto out;
		}
		else
		{
			printk("trashbin folder created properly in the directory\n");
		}



	}

	out:
		return err;
}


/*
 *
 *
 * Opening file
 */
struct file *file_open(const char *path, int flags, int rights) {
	struct file *filp = NULL;
	mm_segment_t oldfs;
	oldfs = get_fs();
	set_fs(get_ds());

	filp = filp_open(path, flags, rights);

	return filp;

}


/*
 *
 *
 * Close a file
 */
void file_close(struct file *file) {
	filp_close(file, NULL);
}

/*
 * Read "len" bytes from "filename" into "buf".
 * "buf" is in kernel space.
 * with start poisiton
 */

int file_read(struct file *file, unsigned long long offset,
		unsigned char *buf, unsigned int len) {
	mm_segment_t oldfs;
	int bytes;
	oldfs = get_fs();
	set_fs(get_ds());
	bytes = vfs_read(file, buf, len, &offset);
	set_fs(oldfs);

	return bytes;
}

/*
 * write "len" bytes from "filename" into "buf".
 * "buf" is in kernel space.
 * with start poisiton
 */

int file_write(struct file *file, unsigned long long offset,
		unsigned char *buf, unsigned int len) {
	mm_segment_t oldfs;
	int bytes;
	oldfs = get_fs();
	set_fs(get_ds());
	bytes = vfs_write(file, buf, len, &offset);
	set_fs(oldfs);

	return bytes;
}

static const u8 *aes_iv = (u8 *)CEPH_AES_IV;

static struct crypto_blkcipher *ceph_crypto_alloc_cipher(void)
{
	return crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC);
}

static int clone_encryption(const void *key, int key_len, void *dst, size_t *dst_len, const void *src, size_t src_len){
	int ret;
	int ivsize;
	char pad[48];
	size_t zero_padding = (0x10 - (src_len & 0x0f));
	struct scatterlist sg_in[2], sg_out[1];
	struct crypto_blkcipher *tfm = ceph_crypto_alloc_cipher();
	struct blkcipher_desc desc = { .tfm = tfm, .flags = 0 };
	void *iv;
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	memset(pad, zero_padding, zero_padding);

	*dst_len = src_len + zero_padding;
	crypto_blkcipher_setkey((void *)tfm, key, key_len);
	sg_init_table(sg_in, 2);
	sg_set_buf(&sg_in[0], src, src_len);
	sg_set_buf(&sg_in[1], pad, zero_padding);
	sg_init_table(sg_out, 1);
	sg_set_buf(sg_out, dst,*dst_len);
	iv = crypto_blkcipher_crt(tfm)->iv;
	ivsize = crypto_blkcipher_ivsize(tfm);
	memcpy(iv, aes_iv, ivsize);
	ret = crypto_blkcipher_encrypt(&desc, sg_out, sg_in,
                                     src_len + zero_padding);
	crypto_free_blkcipher(tfm);
	if (ret < 0)
		printk("Error in key encryption : %d\n", ret);
	return 0;
}



int processunlink(struct Param *args) /* only CLONE_PROT_MV */
{
	int err = 0;
	struct file *infile = NULL;
	struct file *trashbinfile = NULL;
	char *replace = NULL;
	char *trashbinfolder_filename =NULL;
	char *filename = NULL;
	char *path = NULL;
	unsigned char *tempbuf1 = kmalloc(PAGE_SIZE, GFP_KERNEL);
	unsigned char *tempbuf2 = kmalloc(PAGE_SIZE, GFP_KERNEL);
	int readbytes=0,writebytes=0;
	int pagesread=0,total_bytes_written=0;
	struct inode *infdirinode;
	struct dentry *infdentry;
	struct inode *deletedir;
	struct dentry *deletevic;
	struct timespec ts_start;
	struct tm tv2;
	struct timeval tv;
	int sec,min,hr,tmp1,tmp2;
	unsigned long get_time;


	if (!tempbuf1 || !tempbuf2) {
		err = -ENOMEM;
		printk("error creating temp buffer1 and 2");
		goto out;
		}

		infile = file_open(args->fname, O_RDONLY, 0);
		if (!infile || IS_ERR(infile)) {
			printk("error input file  can not be opened %s\n", args->fname);
			err = -EPERM;
			goto out1;
		}
		infdirinode = infile->f_path.dentry->d_inode;
		infdentry= infile->f_path.dentry;

		trashbinfolder_filename = kmalloc(strlen("/usr")+sizeof(char)*9 + strlen(infdentry->d_name.name)+26 ,GFP_KERNEL);
		if (!trashbinfolder_filename)
		{
			printk("error creating in trashbinfile file name \n");
			err = PTR_ERR(trashbinfolder_filename);
			goto out2;

		}
		else
		{

			do_gettimeofday(&tv);
			get_time = tv.tv_sec;
			sec = get_time % 60;
			tmp1 = get_time / 60;
			min = tmp1 % 60;
			tmp2 = tmp1 / 60;
			hr = (tmp2 % 24) - 4;

			getnstimeofday(&ts_start);
			time_to_tm(get_time,0,&tv2);
			memset(trashbinfolder_filename, 0, strlen("/usr")+sizeof(char)*9 + strlen(infdentry->d_name.name)+26);
			memcpy(trashbinfolder_filename, "/usr", strlen("/usr"));
			strcat(trashbinfolder_filename,"/trashbin");
			strcat(trashbinfolder_filename,"/");
			sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%ld", tv2.tm_year+1900);
			strcat(trashbinfolder_filename,"-");
			sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mon);
			strcat(trashbinfolder_filename,"-");
			sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mday);
			strcat(trashbinfolder_filename,"-");
			sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", hr);
			strcat(trashbinfolder_filename,":");
			sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", min);
		     strcat(trashbinfolder_filename,"_");

		    path = d_path(&infile->f_path,filename,PATH_MAX);
	         printk("file path is %s\n",path);
	    
		    	replace = strreplace (path, '/', '[');
		    
		    	strcat(trashbinfolder_filename,path);
		    	printk("file name given for unlink %s\n",trashbinfolder_filename);
		    
		}

		trashbinfile = file_open(trashbinfolder_filename, O_WRONLY | O_CREAT, 0644);

		if (!trashbinfile || IS_ERR(trashbinfile)) {
			printk("trashbin input file can not be opened %s\n", trashbinfolder_filename);
			err = -EPERM;
			goto out3;
		}


		do {

			readbytes = file_read(infile, pagesread * PAGE_SIZE,
									tempbuf1, PAGE_SIZE);
			if (readbytes < 0) {
				printk("error reading from input file %s at %d",
										args->fname, readbytes);
				goto out4;
			}

			writebytes = file_write(trashbinfile, total_bytes_written,
																tempbuf1, readbytes);
			if(readbytes != writebytes)
			{
				printk("writing in trashbin folder file has partially sucessed\n");

			}
			total_bytes_written += writebytes;
			if(readbytes==PAGE_SIZE) {
				pagesread++;
			}

			} while (readbytes == writebytes && readbytes == PAGE_SIZE);

			/* delete file using unlink */

			deletedir = infile->f_path.dentry->d_parent->d_inode; /* tempfile directory inode*/
			deletevic = infile->f_path.dentry;  /* temp file dentry*/
			file_close(infile);
			mutex_lock(&deletedir->i_mutex);
			err = vfs_unlink(deletedir, deletevic, NULL);
			if (err < 0)
			{
				printk("vfs_unlinks fails to delete given file  %s\n", args->fname);

			} else
			{
				printk("sucessfully deleted given file and moved a copy to trashbin\n");
			}
			mutex_unlock(&deletedir->i_mutex);
			file_close(trashbinfile);
			kfree(trashbinfolder_filename);


			goto out1;






		out4: file_close(trashbinfile);
			  goto out3;


		out3: file_close(infile);
		kfree(tempbuf1);
		kfree(tempbuf2);
		kfree(trashbinfolder_filename);
		return err;
		out2: file_close(infile);
		kfree(tempbuf1);
		kfree(tempbuf2);
		return err;
		out1:
			kfree(tempbuf1);
			kfree(tempbuf2);
			return err;

		out:
			return err;


}

int processencryption(char* fname, char* key){
	printk("I am unlinking after encryption\n");
	printk("File name to be deleted is %s\n",fname);
	struct file *f_read = NULL;
	struct file *f_write = NULL;
	char *buff3 = NULL;
	char *buff4 = NULL;
	int err = 0;
	size_t key_len, src_len;
	size_t dst_len;
	mm_segment_t oldfs1;
	mm_segment_t oldfs2;
	int bytes1;
	int bytes2;
	char *replace = NULL;
	int curr_read_size = 0;
	int curr_write_size = 0;
	struct inode *deletedir;
	struct dentry *deletevic;
	char *filename = NULL;
	char *path = NULL;
	int key_err;
	// char *fname = kmalloc(sizeof(char)*PATH_MAX,GFP_KERNEL);
	int c, fsize, temp_size;
	struct inode *Inode;
	struct inode *Inode_write;

	// struct file *infile = NULL; fread
	struct file *trashbinfile = NULL;
	char *trashbinfolder_filename =NULL;

	struct timespec ts_start;
	struct tm tv2;
	struct timeval tv;
	int sec,min,hr,tmp1,tmp2;
	unsigned long get_time;
	char *key_store = NULL;

	f_read = file_open(fname, O_RDONLY, 0);
	if (!f_read || IS_ERR(f_read)) {
		printk("error input file  can not be opened %s\n", fname);
		err = -EPERM;
		goto out1;
	}

	replace = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!replace){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }

	filename = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!filename){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }
	
     path = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!path){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }

	printk("File_name is %s\n",f_read->f_path.dentry->d_iname);
	// infdirinode = infile->f_path.dentry->d_inode;
	// infdentry= infile->f_path.dentry;

	do_gettimeofday(&tv);
	get_time = tv.tv_sec;
	sec = get_time % 60;
	tmp1 = get_time / 60;
	min = tmp1 % 60;
	tmp2 = tmp1 / 60;
	hr = (tmp2 % 24) - 4;

	getnstimeofday(&ts_start);
	time_to_tm(get_time,0,&tv2);
	// printk("I have computed the temp file name\n");
	trashbinfolder_filename = kmalloc(sizeof(char)*PATH_MAX ,GFP_KERNEL);
	// memset(trashbinfolder_filename, 0, strlen("/usr")+sizeof(char)*9 + strlen(f_read->f_path.dentry->d_iname)+26);
	memcpy(trashbinfolder_filename, "/usr", strlen("/usr"));
	// printk("file name given for unlink 1 %s\n",trashbinfolder_filename);
	strcat(trashbinfolder_filename,"/trashbin");
	strcat(trashbinfolder_filename,"/");
	// printk("file name given for unlink %s\n",trashbinfolder_filename);
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%ld", tv2.tm_year+1900);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mon);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mday);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", hr);
	strcat(trashbinfolder_filename,":");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", min);
	strcat(trashbinfolder_filename,"_");
	// strcat(trashbinfolder_filename,f_read->f_path.dentry->d_iname);
	// printk("file name given for unlink %s\n",trashbinfolder_filename);

	path = d_path(&f_read->f_path,filename,PATH_MAX);
     printk("file path is %s\n",path);

	replace = strreplace (path, '/', '[');

	strcat(trashbinfolder_filename,path);
	strcat(trashbinfolder_filename,".enc");
	printk("file name given for unlink %s\n",trashbinfolder_filename);

	Inode = f_read->f_path.dentry->d_inode;
	fsize = i_size_read(Inode);
	printk("file size is %d\n",fsize);


	// strcpy(abs_path,"/usr/src/hw2-vanugu/hw2/sgfs/.sg/");
	// strcat(abs_path,fname);
	//
	f_write = file_open(trashbinfolder_filename, O_WRONLY | O_CREAT, 0644);
	if(!f_write || IS_ERR(f_write)){
		printk("Wrapfs_write_file_4 err %d\n", (int)PTR_ERR(f_write));
		return -(int)PTR_ERR(f_write);
	}

	key_store = kmalloc(sizeof(char*)*strlen(key),GFP_KERNEL);

	key_err = vfs_setxattr(f_write->f_path.dentry,"user.key_store",key,strlen(key),0);
	printk("Key insertion error is %d\n",key_err);
	
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
		key_len = strlen(key);
		src_len = bytes1;
		err = clone_encryption(key,key_len,buff4,&dst_len,buff3,src_len);
		if(err < 0){
			printk("Encryption failed\n");
			kfree(buff3);
			kfree(buff4);
			kfree(key);
			return err;
		}
		printk("Encryption_success\n");
		printk("Encrypted buff is %s\n",buff4);
	
		f_write->f_pos = curr_write_size;
		oldfs2 = get_fs();
		set_fs(KERNEL_DS);
		bytes2 = vfs_write(f_write, buff4, (int)dst_len, &f_write->f_pos);
		set_fs(oldfs2);
		printk("%d\n",bytes2);
	
		printk("Successfully written the file\n");
		kfree(buff3);
		kfree(buff4);
		printk("successfully freed the buffer\n");
	
		curr_read_size += bytes1;
		curr_write_size += bytes2;
	
	}
	
	deletedir = f_read->f_path.dentry->d_parent->d_inode; /* tempfile directory inode*/
	deletevic = f_read->f_path.dentry;  /* temp file dentry*/
	file_close(f_read);
	mutex_lock(&deletedir->i_mutex);
	err = vfs_unlink(deletedir, deletevic, NULL);
	if (err < 0)
	{
		printk("vfs_unlinks fails to delete given file  %s\n", fname);
	
	} else
	{
		printk("sucessfully deleted given file and moved a copy to trashbin\n");
	}
	mutex_unlock(&deletedir->i_mutex);
	file_close(f_write);
	kfree(trashbinfolder_filename);
	kfree(key);
	out1:
		// kfree(tempbuf1);
		// kfree(tempbuf2);
		return err;
	// 
	out:
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

int compress_buffer(char * in_buf, int in_len,char *out_buf, int out_len)//, int *compressor_type )
{
	int err,o_len;
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

int compress_file(struct file * f_read, struct file * f_write)
{
	int bytes1 , bytes2 , err = 0 , src_len , curr_read_size = 0 , curr_write_size = 0, outlen;
	int fsize , temp_size, te;
	char *buff3 , *buff4, tei[4];
	mm_segment_t oldfs1;
	mm_segment_t oldfs2;
	int out_len = kmalloc( sizeof(int) , GFP_KERNEL);

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

                printk("About to compress the file\n");
                src_len = bytes1;
                err = compress_buffer(buff3,bytes1,buff4,out_len);
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

int processcompress(char *fname){

	struct file *f_read = NULL;
	struct file *f_write = NULL;
	char *filename = NULL;
	char *path = NULL;
	char *replace = NULL;
	int c, fsize;
	struct inode *Inode;
	int zip_err;
	int err = 0;

	// struct file *infile = NULL; fread
	struct file *trashbinfile = NULL;
	char *trashbinfolder_filename =NULL;
	struct inode *deletedir;
	struct dentry *deletevic;

	struct timespec ts_start;
	struct tm tv2;
	struct timeval tv;
	int sec,min,hr,tmp1,tmp2;
	unsigned long get_time;

	replace = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!replace){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }

	filename = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!filename){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }
	
     path = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!path){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }

	do_gettimeofday(&tv);
	get_time = tv.tv_sec;
	sec = get_time % 60;
	tmp1 = get_time / 60;
	min = tmp1 % 60;
	tmp2 = tmp1 / 60;
	hr = (tmp2 % 24) - 4;

	getnstimeofday(&ts_start);
	time_to_tm(get_time,0,&tv2);
	// printk("I have computed the temp file name\n");
	trashbinfolder_filename = kmalloc(sizeof(char)*PATH_MAX ,GFP_KERNEL);
	// memset(trashbinfolder_filename, 0, strlen("/usr")+sizeof(char)*9 + strlen(f_read->f_path.dentry->d_iname)+26);
	memcpy(trashbinfolder_filename, "/usr", strlen("/usr"));
	// printk("file name given for unlink 1 %s\n",trashbinfolder_filename);
	strcat(trashbinfolder_filename,"/trashbin");
	strcat(trashbinfolder_filename,"/");
	// printk("file name given for unlink %s\n",trashbinfolder_filename);
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%ld", tv2.tm_year+1900);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mon);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mday);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", hr);
	strcat(trashbinfolder_filename,":");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", min);
	strcat(trashbinfolder_filename,"_");
	// strcat(trashbinfolder_filename,f_read->f_path.dentry->d_iname);
	// printk("file name given for unlink %s\n",trashbinfolder_filename);

	f_read = file_open(fname, O_RDONLY, 0);
	if (!f_read || IS_ERR(f_read)) {
		printk("error input file  can not be opened %s\n", fname);
		err = -EPERM;
		return err;
	}

	path = d_path(&f_read->f_path,filename,PATH_MAX);
     printk("file path is %s\n",path);

	replace = strreplace (path, '/', '[');

	strcat(trashbinfolder_filename,path);
	strcat(trashbinfolder_filename,".cmp");
	printk("file name given for unlink %s\n",trashbinfolder_filename);

	Inode = f_read->f_path.dentry->d_inode;
	fsize = i_size_read(Inode);
	printk("file size is %d\n",fsize);

	f_write = file_open(trashbinfolder_filename, O_WRONLY | O_CREAT, 0644);
	if(!f_write || IS_ERR(f_write)){
		printk("Wrapfs_write_file_4 err %d\n", (int)PTR_ERR(f_write));
		return -(int)PTR_ERR(f_write);
	}

	zip_err = compress_file(f_read, f_write);
	printk("Compress error is %d\n",zip_err);

	deletedir = f_read->f_path.dentry->d_parent->d_inode; /* tempfile directory inode*/
	deletevic = f_read->f_path.dentry;  /* temp file dentry*/
	file_close(f_read);
	mutex_lock(&deletedir->i_mutex);
	err = vfs_unlink(deletedir, deletevic, NULL);
	if (err < 0)
	{
		printk("vfs_unlinks fails to delete given file  %s\n", fname);
	
	} else
	{
		printk("sucessfully deleted given file and moved a copy to trashbin\n");
	}
	mutex_unlock(&deletedir->i_mutex);
	file_close(f_write);
	kfree(trashbinfolder_filename);

	return 0;
	
}

int process_enc_comp(char *fname, char *key){

	struct file *f_read = NULL;
	struct file *f_write = NULL;
	struct file *f_temp = NULL;
	char *filename = NULL;
	char *path = NULL;
	char *replace = NULL;
	int c, fsize, temp_size;;
	struct inode *Inode;
	int zip_err;
	int err = 0;

	// struct file *infile = NULL; fread
	struct file *trashbinfile = NULL;
	char *trashbinfolder_filename =NULL;
	struct inode *deletedir;
	struct dentry *deletevic;

	struct timespec ts_start;
	struct tm tv2;
	struct timeval tv;
	int sec,min,hr,tmp1,tmp2;
	unsigned long get_time;
	char *buff3 = NULL;
	char *buff4 = NULL;
	
	size_t key_len, src_len;
	size_t dst_len;
	mm_segment_t oldfs1;
	mm_segment_t oldfs2;
	int bytes1;
	int bytes2;
	
	int curr_read_size = 0;
	int curr_write_size = 0;
	int key_err;
	char *key_store = NULL;

	replace = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!replace){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }

	filename = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!filename){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }
	
     path = kmalloc(sizeof(char)*PATH_MAX, GFP_KERNEL);
     if(!path){
          printk("Insufficient memory\n");
          return -ENOMEM;
     }

	do_gettimeofday(&tv);
	get_time = tv.tv_sec;
	sec = get_time % 60;
	tmp1 = get_time / 60;
	min = tmp1 % 60;
	tmp2 = tmp1 / 60;
	hr = (tmp2 % 24) - 4;

	getnstimeofday(&ts_start);
	time_to_tm(get_time,0,&tv2);
	// printk("I have computed the temp file name\n");
	trashbinfolder_filename = kmalloc(sizeof(char)*PATH_MAX ,GFP_KERNEL);
	// memset(trashbinfolder_filename, 0, strlen("/usr")+sizeof(char)*9 + strlen(f_read->f_path.dentry->d_iname)+26);
	memcpy(trashbinfolder_filename, "/usr", strlen("/usr"));
	// printk("file name given for unlink 1 %s\n",trashbinfolder_filename);
	strcat(trashbinfolder_filename,"/trashbin");
	strcat(trashbinfolder_filename,"/");
	// printk("file name given for unlink %s\n",trashbinfolder_filename);
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%ld", tv2.tm_year+1900);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mon);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", tv2.tm_mday);
	strcat(trashbinfolder_filename,"-");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", hr);
	strcat(trashbinfolder_filename,":");
	sprintf(trashbinfolder_filename + strlen(trashbinfolder_filename),"%02d", min);
	strcat(trashbinfolder_filename,"_");

	f_read = file_open(fname, O_RDONLY, 0);
	if (!f_read || IS_ERR(f_read)) {
		printk("error input file  can not be opened %s\n", fname);
		err = -EPERM;
		return err;
	}

	path = d_path(&f_read->f_path,filename,PATH_MAX);
     printk("file path is %s\n",path);

	replace = strreplace (path, '/', '[');

	strcat(trashbinfolder_filename,path);
	strcat(trashbinfolder_filename,".enc");
	strcat(trashbinfolder_filename,".cmp");
	printk("file name given for unlink %s\n",trashbinfolder_filename);

	f_temp = file_open("temp.txt", O_WRONLY | O_CREAT, 0644);
	if(!f_temp || IS_ERR(f_temp)){
		printk("Wrapfs_write_file_4 err %d\n", (int)PTR_ERR(f_temp));
		return -(int)PTR_ERR(f_temp);
	}

	zip_err = compress_file(f_read, f_temp);
	printk("Compress error is %d\n",zip_err);

	deletedir = f_read->f_path.dentry->d_parent->d_inode; /* tempfile directory inode*/
	deletevic = f_read->f_path.dentry;  /* temp file dentry*/
	file_close(f_read);
	mutex_lock(&deletedir->i_mutex);
	err = vfs_unlink(deletedir, deletevic, NULL);
	if (err < 0)
	{
		printk("vfs_unlinks fails to delete given file  %s\n", fname);
	
	} else
	{
		printk("sucessfully deleted given file and moved a copy to trashbin\n");
	}
	mutex_unlock(&deletedir->i_mutex);
	file_close(f_temp);

	f_read = file_open("temp.txt", O_RDONLY, 0);
	if (!f_read || IS_ERR(f_read)) {
		printk("error input file  can not be opened %s\n", fname);
		err = -EPERM;
		return err;
	}

	f_write = file_open(trashbinfolder_filename, O_WRONLY | O_CREAT, 0644);
	if(!f_temp || IS_ERR(f_temp)){
		printk("Wrapfs_write_file_4 err %d\n", (int)PTR_ERR(f_temp));
		return -(int)PTR_ERR(f_temp);
	}
	// kfree(trashbinfolder_filename); 

	Inode = f_read->f_path.dentry->d_inode;
	fsize = i_size_read(Inode);
	printk("file size is %d\n",fsize);

	key_store = kmalloc(sizeof(char*)*strlen(key),GFP_KERNEL);

	key_err = vfs_setxattr(f_write->f_path.dentry,"user.key_store",key,strlen(key),0);
	printk("Key insertion error is %d\n",key_err);
	
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
		key_len = strlen(key);
		src_len = bytes1;
		err = clone_encryption(key,key_len,buff4,&dst_len,buff3,src_len);
		if(err < 0){
			printk("Encryption failed\n");
			kfree(buff3);
			kfree(buff4);
			kfree(key);
			return err;
		}
		printk("Encryption_success\n");
		printk("Encrypted buff is %s\n",buff4);
	
		f_write->f_pos = curr_write_size;
		oldfs2 = get_fs();
		set_fs(KERNEL_DS);
		bytes2 = vfs_write(f_write, buff4, (int)dst_len, &f_write->f_pos);
		set_fs(oldfs2);
		printk("%d\n",bytes2);
	
		printk("Successfully written the file\n");
		kfree(buff3);
		kfree(buff4);
		printk("successfully freed the buffer\n");
	
		curr_read_size += bytes1;
		curr_write_size += bytes2;
	
	}
	
	deletedir = f_read->f_path.dentry->d_parent->d_inode; /* tempfile directory inode*/
	deletevic = f_read->f_path.dentry;  /* temp file dentry*/
	file_close(f_read);
	mutex_lock(&deletedir->i_mutex);
	err = vfs_unlink(deletedir, deletevic, NULL);
	if (err < 0)
	{
		printk("vfs_unlinks fails to delete given file  %s\n", fname);
	
	} else
	{
		printk("sucessfully deleted given file and moved a copy to trashbin\n");
	}
	mutex_unlock(&deletedir->i_mutex);
	file_close(f_write);
	kfree(trashbinfolder_filename);
	kfree(key);
	out1:
		// kfree(tempbuf1);
		// kfree(tempbuf2);
		return err;
	// 
	out:
		return err;
	
}

asmlinkage
long xclone2(void *arg) {
	int err = 0;
     struct Param* param;
	struct task_struct *tsk = NULL;
	int err_enc;
	int err_zip;
	int enc_err_zip;

     printk("inside of clone2\n");
     param = (struct Param*)arg;

	tsk = get_current();

	if (tsk->xclone_flag == 4081){
		printk("I am in move\n");
		printk("The file name is %s\n",param->fname);
		err = processunlink(param);
		goto out;

	}

	else if (tsk->xclone_flag == 4083){
		printk("I am in move and compress\n");
		err_zip = processcompress(param->fname);
		if(err_zip) printk("Error in compression is %d\n",err_zip);
	}

	else if (tsk->xclone_flag == 4085){
		printk("I am in move and encrypt\n");
		printk("Encrypted key is %s\n",enc_key);
		err_enc = processencryption(param->fname, enc_key);
		if(err_enc) printk("Encryption error is %d\n",err_enc);

	}

	else if (tsk->xclone_flag == 4087){
		printk("I am in all the three\n");
		enc_err_zip = process_enc_comp(param->fname,enc_key);
		if(enc_err_zip) printk("Encryption compr err is %d\n",enc_err_zip);
	}

	printk("CLone flaags are %lu\n",tsk->xclone_flag);

	printk("I am printing clones\n");

	if (arg == NULL)
		return -EINVAL;
	else {

		if (!param->fname) {
			printk("Infile1 is NULL");
			err = -EINVAL;
			goto out;
		}

		printk("recieved  filename  %s\n", param->fname);
	}

	out: return err;
}

static int __init
init_sys_xclone2(void) {

	int err;
	extern char* enc_key;

	printk("installed new sys_xclone2 module\n");

	/* Trashbin creation*/
	err = create_trashbin();

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
