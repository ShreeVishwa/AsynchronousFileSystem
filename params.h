struct Param{ 
	char *fname;
	u_int b1;
};
// extern int processunlink(struct Param *args);
// extern int process_enc_comp(char *fname, char *key);
// extern struct file *file_open(const char *path, int flags, int rights);
// extern void file_close(struct file *file);
// extern int file_read(struct file *file, unsigned long long offset,
// 		unsigned char *buf, unsigned int len);
// extern int file_write(struct file *file, unsigned long long offset,
// 		unsigned char *buf, unsigned int len);
// extern int processcompress(char *fname);
// extern int compress_file(struct file * f_read, struct file * f_write);
// extern int compress_buffer(char * in_buf, int in_len,char *out_buf, int out_len);
// extern int processencryption(char* fname, char* key);
// extern int processunlink(struct Param *args);
// extern static int clone_encryption(const void *key, int key_len, void *dst, size_t *dst_len, const void *src, size_t src_len);
// extern static struct crypto_blkcipher *ceph_crypto_alloc_cipher(void);
