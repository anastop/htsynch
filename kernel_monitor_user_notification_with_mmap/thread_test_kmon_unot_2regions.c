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

#define SLEEP_TIME_PACK0    5
#define NOTIFY_TIME_PACK0   15
#define SLEEP_TIME_PACK1    8
#define NOTIFY_TIME_PACK1   13
#define END_TIME    20
    
extern cpus_t cpus;
    
typedef struct targs_st {
    my_cpuset_t cpuset;
    int id;
    int lsense;
    int package;
    int sleep_time;
    int notify_time;
} targs_t;
    
targs_t ta0, ta1, ta2, ta3;

/*pthread_barrier_t barrier;*/

/*
 * Pointers to first and second memory region.
 * They respectively show to the same locations 
 * as pointers mwmon_mmap_area and mwmon_mmap_area2
 * in kernel space.
 */ 
char *mmapped_device_memory;
char *mmapped_device_memory2;

/* Distance (in bytes) between the 2 monitored memory regions */
#define MONITORED_REGIONS_DISTANCE 512

#ifdef KERNEL_2_6_13_1 
long int *lint_counter_addr;
#endif


void*  sleeper(void *arg)
{
    targs_t *ta = (targs_t*)arg;
    int i;

    void (*my_mwmon_mmap_sleep_func)(void);
    char *my_mwmon_mmap_area;

    if(ta->id==0) {
        my_mwmon_mmap_sleep_func = fast_mwmon_mmap_sleep;
        my_mwmon_mmap_area = mmapped_device_memory;
    } else if(ta->id==2) {
        my_mwmon_mmap_sleep_func = fast_mwmon_mmap_sleep2;
        my_mwmon_mmap_area = mmapped_device_memory2;
    }
    
    my_sched_setaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    my_sched_getaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    fprintf(stderr, "Sleeper. Tid %d, cpuset %ld.\n", gettid(), ta->cpuset);

    SPIN_BARRIER_LSENSE(barrier, ta->lsense);

    for(i=0; i<END_TIME; i++) {
        fprintf(stderr, "[Sleeper (package %d, set %ld)]: %d\n", 
                ta->package, ta->cpuset, i);
        sleep(1);
        if(i==ta->sleep_time) 
            my_mwmon_mmap_sleep_func(); 
    }
    
    switch(*my_mwmon_mmap_area) {
        case MWMON_NOTIFIED_VAL:
            fprintf(stderr, "[Sleeper (package %d)]: monitored mmapped "
                    "device memory at notified value\n",
                    ta->package);
            break;

        case MWMON_ORIGINAL_VAL:
            fprintf(stderr, "[Sleeper (package %d)]: monitored mmapped "
                    "device memory at original value\n",
                    ta->package);
            break;

        default:
            fprintf(stderr, "[Sleeper (package %d)]: Illegal value for "
                    "monitored mmapped device memory\n",
                    ta->package);
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
        
    char *my_mwmon_mmap_area;

    if(ta->id==1) {
        my_mwmon_mmap_area = mmapped_device_memory;
    } else if(ta->id==3) {
        my_mwmon_mmap_area = mmapped_device_memory2;
    }

    my_sched_setaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    my_sched_getaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    fprintf(stderr, "Notifier. Tid %d, cpuset %ld.\n", gettid(), ta->cpuset);
    
    SPIN_BARRIER_LSENSE(barrier, ta->lsense);

    for(i=0; i<END_TIME; i++) {
        fprintf(stderr, "[Notifier (package %d, set %ld)]: %d\n", 
                ta->package, ta->cpuset, i);
        sleep(1);

        if(i==ta->notify_time)
            *my_mwmon_mmap_area = MWMON_NOTIFIED_VAL;  
    }


    switch(*my_mwmon_mmap_area) {
        case MWMON_NOTIFIED_VAL:
            fprintf(stderr, "[Notifier (package %d)]: monitored mmapped "
                    "device memory at notified value\n",
                    ta->package);
            break;

        case  MWMON_ORIGINAL_VAL:
            fprintf(stderr, "[Notifier (%d)]: monitored mmapped device "
                    "memory at original value\n",
                    ta->package);
            break;

        default:
            fprintf(stderr, "[Notifier (%d)]: Illegal value for monitored "
                    "mmapped device memory\n",
                    ta->package);
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
    pthread_t t0,t1,t2,t3;
    int fd, len;

    ta0.id = 0; 
    ta1.id = 1;
    ta2.id = 2;
    ta3.id = 3;

    ta0.package=0;
    ta1.package=0;
    ta2.package=1;
    ta3.package=1;

    ta0.sleep_time = SLEEP_TIME_PACK0;
    ta1.notify_time = NOTIFY_TIME_PACK0;
    ta2.sleep_time = SLEEP_TIME_PACK1;
    ta3.notify_time = NOTIFY_TIME_PACK1;

    /*
     * open device
     */ 
    len = NPAGES * getpagesize();

    if ((fd = open("/dev/kmem_mapper", O_RDWR | O_SYNC)) < 0) {
        perror("open");
        exit(-1);
    }

    /*
     *  map device memory to userspace
     */ 
    mmapped_device_memory = mmap(0, 
            len, PROT_READ | PROT_WRITE, 
            MAP_SHARED | MAP_LOCKED, 
            fd, 
            len);
    if (mmapped_device_memory == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    mmapped_device_memory2 = mmapped_device_memory + MONITORED_REGIONS_DISTANCE;
    fprintf(stderr, "Mmap address (first monitored mem. region address): 0x%lx\n", 
            (unsigned long)mmapped_device_memory);
    fprintf(stderr, "Second monitored mem. region address: 0x%lx\n", 
            (unsigned long)mmapped_device_memory2);

#ifdef KERNEL_2_6_13_1
    lint_counter_addr = (long int*)(mmapped_device_memory + MWMON_LINT_VAR_OFFSET);
#endif

    /*pthread_barrier_init(&barrier, NULL, 4);*/
    spin_barrier_init(&barrier, 4);
    spin_barrier_init_lsense(&ta0.lsense);
    spin_barrier_init_lsense(&ta1.lsense);
    spin_barrier_init_lsense(&ta2.lsense);
    spin_barrier_init_lsense(&ta3.lsense);

    MY_CPU_ZERO(&ta0.cpuset);
    MY_CPU_ZERO(&ta1.cpuset);
    MY_CPU_ZERO(&ta2.cpuset);
    MY_CPU_ZERO(&ta3.cpuset);
    MY_CPU_SET(cpus.pk[0].sib[0].cpu_number, &ta0.cpuset);
    MY_CPU_SET(cpus.pk[0].sib[1].cpu_number, &ta1.cpuset);
    MY_CPU_SET(cpus.pk[1].sib[0].cpu_number, &ta2.cpuset);
    MY_CPU_SET(cpus.pk[1].sib[1].cpu_number, &ta3.cpuset);

    pthread_create(&t0, NULL, sleeper, (void*)&ta0);
    pthread_create(&t1, NULL, notifier, (void*)&ta1);
    pthread_create(&t2, NULL, sleeper, (void*)&ta2);
    pthread_create(&t3, NULL, notifier, (void*)&ta3);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    
    /*pthread_barrier_destroy(&barrier); */

    *mmapped_device_memory = MWMON_NOTIFIED_VAL;  
    *mmapped_device_memory2 = MWMON_NOTIFIED_VAL;  

    return 0;
}
