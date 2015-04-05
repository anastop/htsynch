#include <stdio.h>
#include "syscall_macros.h"

#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>

#if defined KERNEL_2_6_13_1
    #define __NR_cpu_halt 256
#elif defined KERNEL_2_6_24_2
    #define __NR_cpu_halt 286
#endif

_syscall0(int, cpu_halt)

int main(int argc, char **argv)
{
    int result=20;

    /*__asm__ volatile ("int $0x80" \*/
        /*: "=a" (result) \*/
        /*: "0" (286)); \*/
    
    result = cpu_halt();
    perror("cpu_halt:");

    fprintf(stderr, "result: %d\n", result);

    return 0;
}
