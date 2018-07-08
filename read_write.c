asmlinkage extern long (*sysptr)(void *arg);

asmlinkage long xdedup(void *arg)
{
	struct Param* param ;//= (struct Param*)kmalloc(sizeof(struct Param),GFP_KERNEL);
	struct file *filp1;
	struct file *filp2;
	struct file *filp3;
	struct file *filp4;
	mm_segment_t oldfs1;
	mm_segment_t oldfs2;
	mm_segment_t oldfs3;
	int bytes1;
	int bytes2;
	int bytes3;
	int i;
	struct kstat stats1;
	struct kstat stats2;
	struct kstat stats3;
	char *buff3;
	char *buff4;
	int error1, error2, error3;
	int file_size = 0;
	int kern_int1,kern_int2;
	struct path path1, path2, path3, path4;
	struct dentry* link_dentry;
	int unlink_err1, link_err1;
	char* temp_name;
	int kern_temp_int1, kern_temp_int2;
	struct dentry* src;
	struct dentry* dst;
	int vfs_ret_val;
	int c;
	int kern_path_sb1, kern_path_sb2, kern_path_sb3;
	struct path path_sb1, path_sb2, path_sb3;
	int unlink_temp_err;
	struct timespec now;
	unsigned short out_mode = 0644;
	int d_flag;
	int copy_user;
	
		
	//char buff3[20];
	param = (struct Param*)arg;
while(file_size < stats1.size){
			
			buff3 = (char*)kmalloc(sizeof(char)*4096,GFP_KERNEL); 	
			buff4 = (char*)kmalloc(sizeof(char)*4096,GFP_KERNEL);

			
    			oldfs1 = get_fs();
    			set_fs(KERNEL_DS);
    			bytes1 = vfs_read(filp1, buff3, 4096, &filp1->f_pos);
    			set_fs(oldfs1);
			//printk("%d\n",bytes1);
			//printk("%s\n",buff3);

    			// close the file 
    			//filp2->f_pos = file_size;
			oldfs2 = get_fs();
			set_fs(KERNEL_DS);
			bytes2 = vfs_read(filp2,buff4,4096,&filp2->f_pos);
			set_fs(oldfs2);
			
			i = 0;
			while(*(buff3+i) != '\0' && i < 4096 && *(buff4+i) != '\0'){
				//printk("I am comapring\n");
				if(*(buff3+i) != *(buff4+i)) break;
				i++;
			}

			if(i == 0 && file_size == 0){
				
				filp4 = filp_open(param->outfile,O_WRONLY | O_CREAT, 0644);
				if(!filp4 || IS_ERR(filp4)){
					printk("Wrapfs_read_file_4 err %d\n", (int)PTR_ERR(filp4));
					return -(int)PTR_ERR(filp4);
				}
				return 0;
			}
	

			//printk("Done reading file\n");
			
				//filp3->f_pos = file_size;
			oldfs3 = get_fs();
			set_fs(KERNEL_DS);
	
			bytes3 = vfs_write(filp3,buff3,i,&filp3->f_pos);
			
			if(i != bytes3){
				printk("Error in writing the file\n");
				mutex_lock_nested(&(path3.dentry)->d_parent->d_inode->i_mutex, I_MUTEX_PARENT);
				unlink_temp_err = vfs_unlink((path3.dentry)->d_parent->d_inode,path3.dentry,NULL);	
				mutex_unlock(&(path3.dentry)->d_parent->d_inode->i_mutex);
				printk("Temp file deleted\n");
				if(unlink_temp_err){
					printk("Unlinking the temp file failed\n");
					return -unlink_temp_err;
				}

				return -1004;
	
			}

			//filp_close(filp3,NULL);

					

			if(i < 4096 && file_size + i != stats1.size){
				file_size = file_size + i;
				break;
			}
		
			file_size += bytes1;
			kfree(buff4);
			kfree(buff3);
		
			//printk("Number of bytes read are %d\n",bytes1);


		}