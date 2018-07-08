#include <linux/ioctl.h>

#define IOC_MAGIC 'a'

#define IOCTL_TB_UNDELETE _IOW(IOC_MAGIC,IOC_MAGIC,int32_t*)
