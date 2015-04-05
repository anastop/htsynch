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
            mwmon_kernel_sleep();
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
            mwmon_kernel_notify();  
        
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    pthread_t t0,t1;

    ta0.id = 0;
    ta1.id = 1;

    mwmon_kernel_init();    

    pthread_create(&t0, NULL, sleeper, (void*)&ta0);
    pthread_create(&t1, NULL, notifier, (void*)&ta1);
    pthread_join(t0, NULL);
    pthread_join(t1, NULL);
    
    mwmon_kernel_finalize();    

    return 0;
}
