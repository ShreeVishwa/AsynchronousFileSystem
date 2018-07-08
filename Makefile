# obj-m += sys_xclone2.o

obj-m += proc_queue_len.o
obj-m += proc_file_num.o
obj-m += proc_key.o
# obj-m += sys_xclone2.o
obj-m += sys_xclone2async.o
# obj-m += sys_deletethread.o
	


INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: xdedupu xdedupa tbctlio

xdedupu: xclone2.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi xclone2.c -o xclone2

xdedupa:
	make -Wall -Werror -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

tbctlio: tbctl.c
	gcc -Wall -Werror -I$(INC)/generated/uapi -I$(INC)/uapi tbctl.c -o tbctl

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f xclone2
	rm -f tbctl
