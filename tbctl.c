#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include "tbctl.h"

int main(int argc, char * const argv[]) {
    int rc = 0;
    int c;
    int fd;
    const char *filename;

    if(argc<3 || argc >3)
    {
        printf("invalid number of arguments for tbctl\n");
        printf("Usage: ./tbctl [-u] FILE");
        return rc;
    }
    while ((c = getopt(argc, argv, "u:")) != -1) {
        switch (c) {
        case 'u':
            if (argc == 3)
            {
                filename = argv[optind-1];
                printf("%s\n", filename);
            }
            break;


        case '?':
            if (optopt == 'u' ) {
                perror("tbctl: option requires an argument \n");
                perror("Usage: ./tbctl [-u] FILE");
            } else {
                perror("tbctl: invalid option \n");
                perror("Usage: tbctl [-u] FILE \n");
            }
            return 1;
        default:
            abort();

        }
    }

    fd=open(filename,O_RDWR|O_CREAT);
    if(fd<0)
    {
        printf("error in creating the file\n");
        return rc;

    }
    rc=ioctl(fd,IOCTL_TB_UNDELETE,0);
    printf("%d",rc);
    close(fd);

 return rc;

}
