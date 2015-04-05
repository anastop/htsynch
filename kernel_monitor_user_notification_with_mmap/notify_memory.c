#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/*#include <sys/types.h>*/
#include <linux/unistd.h> 
#include <errno.h>

#include "cpu.h"
#include "cpu_control_syscalls.h"

#include "syscall_macros.h"
_syscall0(pid_t,gettid)


#define NPAGES 1


char *mmapped_device_memory;


int main(int argc, char **argv)
{
    int fd, len, i;

    /*
     * open device
     */ 
    len = NPAGES * getpagesize();

    if ((fd = open("/dev/kmem_mapper", O_RDWR | O_SYNC)) < 0) {
        perror("open");
        exit(-1);
    }

    /*
     *  map device memory yo userspace
     */ 
    mmapped_device_memory = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, len);
    if (mmapped_device_memory == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }

    for(i=0; i<atoi(argv[1]); i++)
        *mmapped_device_memory = MWMON_NOTIFIED_VAL;  

    return 0;
}
