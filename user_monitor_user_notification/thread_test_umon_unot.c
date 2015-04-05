#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/types.h>
#include <linux/unistd.h> 

#include "cpu.h"
#include "cpu_control_syscalls.h"

#include "syscall_macros.h"
_syscall0(pid_t,gettid)


#define SLEEP_TIME  5
#define NOTIFY_TIME 10
#define END_TIME    20
    
extern cpus_t cpus;
    
typedef struct targs_st {
    my_cpuset_t cpuset;
    int id;
} targs_t;
    
targs_t ta0, ta1;

char *mwmon_memory;

/*
 *  * Allocate 'size' bytes, aligned at an address that
 *   * is a multiple of 'alignment'
 *    */
void* malloc_aligned(size_t size, size_t alignment)
{
    void *p;
    
    if(!(p = malloc(size + alignment-1))) {
        perror("Error in memory allocation!");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stderr, "initial allocation (unaligned) at %p\n", p);
    //Perform the actual alignment: 'alignment' must be
    //a power of 2

    p = (void *) (((unsigned long)p + alignment - 1) & ~(alignment - 1));
    
    return p;
}



void* sleeper(void *arg)
{
    targs_t *ta = (targs_t*)arg;
    int fd, i;
    
    
    MY_CPU_ZERO(&ta->cpuset);
    MY_CPU_SET(cpus.pk[0].sib[0].cpu_number, &ta->cpuset);
    my_sched_setaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    my_sched_getaffinity(gettid(), sizeof(ta->cpuset), &ta->cpuset);
    fprintf(stderr, "tid %d: cpuset: %ld\n", gettid(), ta->cpuset);
    

    for(i=0; i<END_TIME; i++) {
        fprintf(stderr, "[S]: %d\n", i);
        sleep(1);
        if(i==SLEEP_TIME)
            mwmon_user_sleep(mwmon_memory);
    }
    
    switch(*mwmon_memory) {
        case MWMON_NOTIFIED_VAL:
            fprintf(stderr, "[S]: mwmon_memory at notified value\n");
            break;

        case  MWMON_ORIGINAL_VAL:
            fprintf(stderr, "[S]: mwmon_memory at original value\n");
            break;

        default:
            fprintf(stderr, "[S]: Illegal value for mwmon_memory\n");
            break;
    }


    pthread_exit(NULL);
}

void* notifier(void *arg)
{
    int fd, i;
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
            *mwmon_memory = MWMON_NOTIFIED_VAL;  
    }


    switch(*mwmon_memory) {
        case MWMON_NOTIFIED_VAL:
            fprintf(stderr, "[N]: mwmon_memory at notified value\n");
            break;

        case  MWMON_ORIGINAL_VAL:
            fprintf(stderr, "[N]: mwmon_memory at original value\n");
            break;

        default:
            fprintf(stderr, "[N]: Illegal value for mwmon_memory\n");
            break;
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    pthread_t t0,t1;

    ta0.id = 0;
    ta1.id = 1;

    /* "Init" */
    mwmon_memory = (char*)malloc_aligned(MWMON_MONITORED_MEM_BYTES*sizeof(char), MWMON_ALIGN_BOUNDARY);

    fprintf(stderr, "mwmon_memory starts at %p\n", mwmon_memory);

    pthread_create(&t0, NULL, sleeper, (void*)&ta0);
    pthread_create(&t1, NULL, notifier, (void*)&ta1);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    
    return 0;
}
