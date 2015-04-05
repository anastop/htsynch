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

#include "spin.h"
spin_barrier_t barrier;

#define NPAGES 1

#define SLEEP_TIME  5
#define NOTIFY_TIME 10
#define END_TIME    20
    
extern cpus_t cpus;
    
typedef struct targs_st {
    my_cpuset_t cpuset;
    int id;
    int lsense;
} targs_t;
    
targs_t ta0, ta1;

/*pthread_barrier_t barrier;*/

char *mmapped_device_memory;

#ifdef KERNEL_2_6_13_1 
long int *lint_counter_addr;
#endif

void*  sleeper(void *arg)
{
    targs_t *ta = (targs_t*)arg;
    int i;
    
    
    MY_CPU_ZERO(&ta->cpuset);
    MY_CPU_SET(cpus.pk[0].sib[0].cpu_number, &ta->cpuset);
    my_sched_setaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    my_sched_getaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    fprintf(stderr, "tid %d: cpuset: %ld\n", gettid(), ta->cpuset);
    

    for(i=0; i<END_TIME; i++) {
        fprintf(stderr, "[S]: %d\n", i);
        sleep(1);
        if(i==SLEEP_TIME)
            fast_mwmon_mmap_sleep();
    }
    
    switch(*mmapped_device_memory) {
        case MWMON_NOTIFIED_VAL:
            fprintf(stderr, "[S]: monitored mmapped device memory at notified value\n");
            break;

        case  MWMON_ORIGINAL_VAL:
            fprintf(stderr, "[S]: monitored mmapped device memory at original value\n");
            break;

        default:
            fprintf(stderr, "[S]: Illegal value for monitored mmapped device memory\n");
            break;
    }

#ifdef  KERNEL_2_6_13_1 
    /*pthread_barrier_wait(&barrier);*/
    SPIN_BARRIER_LSENSE(barrier, ta->lsense);

    fprintf(stderr, "===========\n");

    for(i=10; i<20; i++) {
        fprintf(stderr, "[S]: %d\n", i);
        sleep(1);
        fast_mwmon_mmap_spin_while_greater(i);
    }
#endif

    pthread_exit(NULL);
}

void* notifier(void *arg)
{
    int i;
    targs_t *ta = (targs_t*)arg;
        
    MY_CPU_ZERO(&ta->cpuset);
    MY_CPU_SET(cpus.pk[0].sib[1].cpu_number, &ta->cpuset);
    my_sched_setaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    my_sched_getaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    fprintf(stderr, "tid %d: cpuset: %ld\n", gettid(), ta->cpuset);
    
    
    for(i=0; i<END_TIME; i++) {
        fprintf(stderr, "[N]: %d\n", i);
        sleep(1);

        if(i==NOTIFY_TIME)
            *mmapped_device_memory = MWMON_NOTIFIED_VAL;  
    }


    switch(*mmapped_device_memory) {
        case MWMON_NOTIFIED_VAL:
            fprintf(stderr, "[N]: monitored mmapped device memory at notified value\n");
            break;

        case  MWMON_ORIGINAL_VAL:
            fprintf(stderr, "[N]: monitored mmapped device memory at original value\n");
            break;

        default:
            fprintf(stderr, "[N]: Illegal value for monitored mmapped device memory\n");
            break;
    }

#ifdef KERNEL_2_6_13_1
    /*pthread_barrier_wait(&barrier);*/
    SPIN_BARRIER_LSENSE(barrier, ta->lsense);

    fprintf(stderr, "===========\n");

    for((*lint_counter_addr)=0; (*lint_counter_addr)<20; (*lint_counter_addr)++) {
        fprintf(stderr, "[N]: %ld\n", (*lint_counter_addr));
        sleep(1);
    }
#endif

    pthread_exit(NULL);

}

int main(int argc, char **argv)
{
    pthread_t t0,t1;
    int fd, len;

    ta0.id = 0;
    ta1.id = 1;

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
    fprintf(stderr, "Mmap address: 0x%lx\n", (unsigned long)mmapped_device_memory);

#ifdef KERNEL_2_6_13_1
    lint_counter_addr = (long int*)(mmapped_device_memory + MWMON_LINT_VAR_OFFSET);
#endif

    /*pthread_barrier_init(&barrier, NULL, 2);*/
    spin_barrier_init(&barrier, 2);
    spin_barrier_init_lsense(&ta0.lsense);
    spin_barrier_init_lsense(&ta1.lsense);

    pthread_create(&t0, NULL, sleeper, (void*)&ta0);
    pthread_create(&t1, NULL, notifier, (void*)&ta1);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    
    /*pthread_barrier_destroy(&barrier); */

    *mmapped_device_memory = MWMON_NOTIFIED_VAL;  

    return 0;
}
