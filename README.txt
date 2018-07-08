OPERATING SYSTEM CSE506 HOMEWORK 3- Spring 2018

Overview:-


This assignment implements a Linux kernel-based system to handle file processing operations like encryption and/or compression synchronously in case of small files (4KB and smaller) and asynchronously for larger files using work queues. 


It supports a special trashbin folder to store deleted files with support for compression to save space; encryption to protect the data; periodic deletion; as well as support to purge the trashbin folder. 


The filename inside the trashbin indicates the operation performed on hat file. 


Files added :-


1. flags.h :- defines CLONE_PROT_MV, CLONE_PROT_ZIP and CLONE_PROT_ENC flags


2. vars.h :- declares global variables like enc_key (encryption key), queue length (max length of kernel queue), num_files (max                          number of files in the trashbin folder).


3. params.h :- defines a struct param 


4. tbctl.h :- defines macro IOC_MAGIC and IOCTL_TB_UNDELETE.


5. tbctl.c :- Support for recovering a lost file
                          usage: ./tbctl [-u] FILE
                          checks for-
                          (i) correct number of arguments for tbctl
                          (ii) prints filename if correct usage


6. proc_queue_len.c :- set up the required fields of file_operations struct like the read and write methods, contains functions                                          to show the current proc queue length, and set the queue length (show error if arises).Removes the proc entry on exit. 


7. proc_file_num.c :- sets the max_file_num in the proc file.


8. proc_key.c :- writes the encryption key to the proc file. Appropriate error handling is supported. 


9. xclone2.c :- fetch the arguments passed by the user and sets the appropriate flag in the params->b1 field. 
                   Creates the child process with appropriate flags set.
                   Appropriate exception handling: insufficient memory checks,    insufficient number and type of arguments, error in cloning. 


10. sys_xclone2.c :- contains functions for creating a trashbin folder; unlinking a file after encryption; file compression and                         unlinking;  as well as support for all three : compress encrypt unlink (ALL THREE).


11. sys_xclone2async.c :- Installs new sys_xclone2async module for asynchronous file processing operations. 




Usage :- 
                 ./xclone2 [-mce] file1.txt
-m : support for unlinking and moving to trashbin folder
-c : support for compression
-e : support for encryption


Usage examples :- 
1. moving to trashbin 
./xclone2.c -m <infile>


2. Moving and encrypting the input file
 Echo “1789” > /proc/enc_key                    //here, 1789 is the key
./xclone2.c -me <infile>


6. moving, encryption and compression:-
Echo “1789” > /proc/enc_key                        //here, 1789 is the key
./xclone2.c -mce <infile>


7. undeleting a lost file
./xclone2.c -u <infile>


For Detailed Design, please refer to the design.pdf file.


References:-


-http://tuxthink.blogspot.ca/2011/04/wait-queues.html - for overview of work queues
-https://sysplay.in/blog/tag/kernel-threads/
-https://stackoverflow.com/questions/44083139/how-to-sort-files-by-name-in-a-directory-on-linux
-https://www.sanfoundry.com/c-tutorials-qsort-bsearch-functions-use-programs/
-http://www3.cs.stonybrook.edu/~ezk/cse506-s18/lectures/23.txt
-http://man7.org/linux/man-pages/ - for overview to various flags and existing calls
-https://elixir.bootlin.com/linux/latest/source 
-https://gist.github.com/BrotherJing/c9c5ffdc9954d998d1336711fa3a6480
-http://tuxthink.blogspot.com/2013/10/creation-of-proc-entrt-using.html
-http://tuxthink.blogspot.com/2011/02/creating-readwrite-proc-entry.html