#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/page.h>
#include <linux/uaccess.h>
#include <linux/uidgid.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/namei.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/buffer_head.h>
#include <linux/sched.h>
#include <linux/pid_namespace.h>
#include <linux/pid.h>
#include <linux/ctype.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>


#include "sys_xclone2async.h"
#include "params.h"


asmlinkage extern long (*sysptr)(void *arg);
// static struct workqueue_struct;

// struct myargs {
//      char *file;
//      u_int flags;
// };

struct wq_node * process_item(void)
{

    struct wq_node *current_job_item=NULL;
    struct list_head *wq_hlist1, *wq_hlist2;
    list_for_each_safe(wq_hlist1,wq_hlist2,&((*wi_queue->w_qnode).wq_list));//&((*wi_queue->w_qnode).wq_list)
    current_job_item = list_entry(wq_hlist1,struct wq_node,wq_list);

    printk("inside of process_item :%lu\n",current_job_item->wq_data_node->b1);
    if(current_job_item)
    {
    	// printk("\n indsi kjdfbdks");
    		printk("I am insie current_job_item\n");
            list_del(wq_hlist1);
            printk("Input filename is %s",current_job_item->wq_data_node->fname);
            return current_job_item;
    }
    return NULL;
}
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
int processunlink_async(struct Param *args) /* only CLONE_PROT_MV */
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

printk("\n in processunlink_async ");
	if (!tempbuf1 || !tempbuf2) {
		err = -ENOMEM;
		printk("error creating temp buffer1 and 2");
		goto out;
		}
printk("\n in processunlink_async 1");
		infile = file_open(args->fname, O_RDONLY, 0);
		if (!infile || IS_ERR(infile)) {
			printk("error input file  can not be opened %s\n", args->fname);
			err = -EPERM;
			goto out1;
		}
		printk("\n in processunlink_async 3");
		infdirinode = infile->f_path.dentry->d_inode;
		infdentry= infile->f_path.dentry;
printk("\n in processunlink_async 4");
		trashbinfolder_filename = kmalloc(strlen("/usr")+sizeof(char)*9 + strlen(infdentry->d_name.name)+26 ,GFP_KERNEL);
		if (!trashbinfolder_filename)
		{
			printk("error creating in trashbinfile file name \n");
			err = PTR_ERR(trashbinfolder_filename);
			goto out2;

		}
		else
		{
printk("\n in processunlink_async 5");
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
printk("\n in processunlink_async 6");
		    path = d_path(&infile->f_path,filename,PATH_MAX);
	         printk("file path is %s\n",path);
	    printk("\n in processunlink_async 7");
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


int consumer_work_func(void *data)
{

     int err;
     struct wq_node *current_job_item=NULL;
     struct Param *args;// = kmalloc(sizeof(struct Param ),GFP_KERNEL);
        struct task_struct *tsk = NULL;

    printk("\n in consumer_work_func function ");
     while(true) /* more producres than size of work queue*/
        {
            mutex_lock(&wi_queue->wq_mutex);

            if(atomic_read(&(wi_queue->wq_size))==0)
            {
                mutex_unlock(&wi_queue->wq_mutex);

                    wait_event_interruptible(consumer_wq,atomic_read(&(wi_queue->wq_size))>0);

            }

            current_job_item=process_item();

            if(IS_ERR(current_job_item))
            {
                mutex_unlock(&wi_queue->wq_mutex);
                return(PTR_ERR(current_job_item));

            }

            atomic_dec(&(wi_queue->wq_size));
            if(atomic_read(&(wi_queue->wq_size))<WQ_maxsize)
            {
                wake_up_all(&producer_wq);
            }

            mutex_unlock(&wi_queue->wq_mutex);
printk("Infile1 is NULL 1\n");
            /* do the work now */
            tsk = get_current();
printk("Infile1 is NULL 2: %s\n",current_job_item->wq_data_node->fname);
            args->fname =  current_job_item->wq_data_node->fname;
            printk("Infile1 is NULL 3");
            // args->b1 =  current_job_item->wq_data_node->b1;
            if (!args->fname) {
                        printk("Infile1 is NULL");
                        err = -EINVAL;
                        return -1;
                    }printk("Infile1 is NULL 4");

                    printk("in  recieved  filename  %s\n", args->fname);
                    args->b1 = tsk->xclone_flag;
printk("Infile1 is NULL 5");

            if((args->b1 & CLONE_PROT_MV) == CLONE_PROT_MV)
                    {
                    	if (NULL == args || NULL == args -> fname)
                    	{
                    		printk("\n ERROR in args ");

                    		return -1;

                    	}
                    	printk("Infile1 is NULL 6");
                        err=processunlink_async(args);
                        if(err!=0)
                        {
                            printk("job failed\n");
                        }
                        else{
                            printk("job sucess\n");

                        }
                    }

                kfree(current_job_item->wq_data_node);
                kfree(current_job_item);
                schedule();
        }
        return err;
}

int initialise_wq(void)
{
    int i;
    wi_queue = kmalloc(sizeof(struct w_queue), GFP_KERNEL);
    printk("\n in initialise_wq function ");
    if ( wi_queue == NULL )
    {
        return -ENOMEM;
    }

    wi_queue -> w_qnode = kmalloc(sizeof(struct wq_node), GFP_KERNEL);

    if ( wi_queue -> w_qnode == NULL )
    {
        return -ENOMEM;
    }
    INIT_LIST_HEAD(&(wi_queue -> w_qnode -> wq_list));
    mutex_init(&wi_queue -> wq_mutex);
    atomic_set(&(wi_queue -> wq_size), 0);
    // wi_queue -> q_size = 0;
    for (i = 0; i < max_consumers; i++)
        consum[i] = kthread_create(consumer_work_func, NULL, "consumer");

    for (i = 0; i < max_consumers; i++)
     wake_up_process(consum[i]);
    return 0;


}

int insert_into_head_job(struct wq_data *workitem)
{
    int err=0;
    struct wq_node *current_job_item=NULL;
    mutex_lock(&wi_queue->wq_mutex);
    printk("\n in unsert head");
    while(atomic_read(&(wi_queue->wq_size))>=WQ_maxsize)  //more producres than size of work queue
    {
        mutex_unlock(&wi_queue->wq_mutex);
        if(atomic_read(&(t_producers))<=max_producers)
        {
            atomic_inc(&(t_producers));
            wait_event_interruptible(producer_wq,atomic_read(&(wi_queue->wq_size))<WQ_maxsize);
            atomic_dec(&(t_producers));
        }
        else{
            printk("can't throttle more producers\n");
            return -1;
        }

    }

    current_job_item=(struct wq_node *) kmalloc(sizeof(struct wq_node *),GFP_KERNEL);
    if(current_job_item==NULL)
    {
        printk("error\n");
        return -1;
    }
    current_job_item->wq_data_node = workitem;
    list_add_tail(&(current_job_item->wq_list),&(wi_queue->w_qnode->wq_list));
    atomic_inc(&(wi_queue->wq_size));

    return err;

}

asmlinkage long xclone2async(void *arg) {

    int err = 0;
    // struct wq_data *workitem=NULL;
    struct wq_data *current_job;
    // struct wq_data

    struct task_struct *tsk = NULL;


    printk("inside of clone2async\n");

    tsk = get_current();
    printk("Clone flags are %lu\n",tsk->xclone_flag);

    if (arg == NULL)
        return -EINVAL;
    else {

        current_job = (struct wq_data *) arg;
        if (!current_job->fname) {
            printk("Infile1 is NULL");
            err = -EINVAL;
            goto out;
        }

        printk("recieved  filename  %s\n", current_job->fname);

        current_job->b1 = tsk->xclone_flag;

        if((current_job->b1 & CLONE_PROT_MV) == CLONE_PROT_MV)
        {
        	printk("in recieved  flags  %d\n", current_job->b1);
            err=insert_into_head_job(current_job);
            if(err!=0){
                printk("couldn't insert a job into queue\n");
                goto out;
            }
            printk("isacfa recieved  flags  %d\n", current_job->b1);
            wake_up_all(&consumer_wq);
            printk("asnc recieved  flags  %d\n", current_job->b1);
            goto out;
        }

    }

    out: return err;
}




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


static int __init
init_sys_xclone2async(void) {
    int err=0;
    printk("installed new sys_xclone2async module\n");
    /* Trashbin creation*/
    err = create_trashbin();

    err = initialise_wq();

    if(err !=0)
    {
        printk("failed to intialize the work queue\n");
        goto out;
    }
    init_waitqueue_head(&consumer_wq);
    init_waitqueue_head(&producer_wq);

    atomic_set(&(t_producers), 0);

    if (sysptr == NULL)
        sysptr = xclone2async;

out:
    return 0;
}

static void __exit
exit_sys_xclone2async(void) {

        struct wq_node *current_job_item=NULL;
        struct list_head *wq_hlist1=NULL,*wq_hlist2=NULL;
        list_for_each_safe(wq_hlist1,wq_hlist2,&((*wi_queue->w_qnode).wq_list));
        current_job_item=list_entry(wq_hlist1,struct wq_node,wq_list);
        if(current_job_item)
        {
                list_del(wq_hlist1);
        }
        kfree(current_job_item);

    if (sysptr != NULL)
        sysptr = NULL;
    printk("removed sys_xclone2async module\n");
}

module_init( init_sys_xclone2async);
module_exit( exit_sys_xclone2async);
MODULE_LICENSE("GPL");
