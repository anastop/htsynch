#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "btimer_rdtsc64.h"
#include "synch.h"
#include "cpu.h"
#include "syscall_macros.h"
#include "mwait_barrier.h"

/* To send wakeup IPIs via ioctl calls */
#include "cpuctrl.h"    

static _syscall0(pid_t, gettid)

double hz;
extern cpus_t cpus;
typedef struct targs_st {
    my_cpuset_t cpuset;
    unsigned int lsense;
} targs_t;

mytimer_t barrier_wakeup_timer, barrier_call_timer, work_timer;
double wakeup_tics, work_tics, call_tics;
int heavy_work, light_work, iterations;

/* for initial synchronization AND testing of the Pthreads barrier */
pthread_barrier_t pbarrier;

#if defined(SPINPAUSE) || defined(SPINHALT)
spin_barrier_t barrier;
#ifdef SPINHALT
int cpuctrl_fd;
#endif

#elif defined(MWMON)
mwait_barrier_t mwbarrier;
#endif

#define N 10
float A[N][N],B[N][N],C[N][N];
void do_work(int c)
{ 
    int iters,i,j,k;

    for(iters=0; iters<c; iters++) {
        for(i=0; i<N; i++)
            for(j=0; j<N; j++) {
                A[i][j] = 1.45+iters;
                B[i][j] = 3.4;
                C[i][j] = 4.5;
            }

        for(i=0; i<N; i++)
            for(j=0; j<N; j++)
                for(k=0; k<N; k++)
                    A[i][j] += B[i][k]*C[k][j];
    }
}

float A2[N][N],B2[N][N],C2[N][N];
void do_work2(int c)
{ 
    int iters,i,j,k;

    for(iters=0; iters<c; iters++) {
        for(i=0; i<N; i++)
            for(j=0; j<N; j++) {
                A2[i][j] = 1.45+iters;
                B2[i][j] = 3.4;
                C2[i][j] = 4.5;
            }

        for(i=0; i<N; i++)
            for(j=0; j<N; j++)
                for(k=0; k<N; k++)
                    A2[i][j] += B2[i][k]*C2[k][j];
    }
}


//Heavyweight thread (e.g. worker)
void* heavy(void *arg) 
{
    targs_t *targs = (targs_t*)arg;
    unsigned int *lsense = &targs->lsense;
    int i;

    my_sched_setaffinity(gettid(), sizeof(targs->cpuset), &targs->cpuset);

    pthread_barrier_wait(&pbarrier);
    TIMER_START(&work_timer);

    for(i=0; i<iterations; i++) { 

        do_work(heavy_work);
        /*fprintf(stderr, "Heavy ended hard work!\n");*/
    
        /*TIMER_START(&barrier_wakeup_timer);*/
        /*TIMER_START(&barrier_call_timer);*/

#if defined(MWMON)
        mwait_barrier(&mwbarrier);
#elif defined(SPINPAUSE)
        spin_barrier_lsense(&barrier, lsense);
#elif defined(SPINHALT)
        spin_barrier_lsense_cpuhalt_sendIPI(&barrier, lsense);
#elif defined(PTHREAD)
        pthread_barrier_wait(&pbarrier);
#endif
        fprintf(stderr,"Heavy: %d\n", i);
        /*getchar();*/
        /*TIMER_STOP(&barrier_call_timer);*/
        /*call_tics = TIMER_TOTAL(&barrier_call_timer);*/
    }

    TIMER_STOP(&work_timer);
    work_tics = TIMER_TOTAL(&work_timer);

    return NULL;
}
 
//Lightweight th read (e.g. prefetcher)
void* light(void *arg) 
{
    targs_t *targs = (targs_t*)arg;
    unsigned int *lsense = &targs->lsense;
    int i;
    
    my_sched_setaffinity(gettid(), sizeof(targs->cpuset), &targs->cpuset);

    pthread_barrier_wait(&pbarrier);

    for(i=0; i<iterations; i++) {
        do_work2(light_work);
        /*fprintf(stderr, "Light ended short work!\n");*/

#if defined(MWMON)
        mwait_barrier(&mwbarrier);
#elif defined(SPINPAUSE)
        spin_barrier_lsense(&barrier, lsense);
#elif defined(SPINHALT)
        spin_barrier_lsense_cpuhalt_sendIPI(&barrier, lsense);
#elif defined(PTHREAD)
        pthread_barrier_wait(&pbarrier);
#endif
        /*TIMER_STOP(&barrier_wakeup_timer);*/
        /*wakeup_tics = TIMER_TOTAL(&barrier_wakeup_timer);*/
        fprintf(stderr,"Light: %d\n", i);
    }

    return NULL;
} 
 

int main(int argc, char *argv[]) {
    
    unsigned long i, nthreads=2;
    pthread_t *thr;
    targs_t *targs;

    heavy_work = atoi(argv[1]);
    light_work = atoi(argv[2]);
    iterations = atoi(argv[3]);

    hz = read_hz();

    targs = (targs_t*)malloc(nthreads*sizeof(targs_t));
    thr = (pthread_t*)malloc(nthreads*sizeof(pthread_t));

    for(i=0; i<nthreads; i++) 
        MY_CPU_ZERO(&targs[i].cpuset);

    MY_CPU_SET(cpus.pk[0].sib[0].cpu_number, &targs[0].cpuset);
    MY_CPU_SET(cpus.pk[0].sib[1].cpu_number, &targs[1].cpuset);

    pthread_barrier_init(&pbarrier, NULL, nthreads);

#if defined(SPINHALT) || defined(SPINPAUSE)
    spin_barrier_init(&barrier, nthreads);

    for(i=0; i<nthreads; i++) 
        spin_barrier_init_lsense(&targs[i].lsense);
    
#ifdef SPINHALT
    if((cpuctrl_fd = open("/dev/cpuctrl", O_RDONLY)) < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
#endif

#endif

#ifdef MWMON
    mwait_barrier_init(&mwbarrier, nthreads);

    /*
     * kmem_mapper actions
     */
    /*
     * open device
     */ 
    kmem_mapped_area_len = MWAIT_NPAGES * getpagesize();
    if ((kmem_mapper_fd = open("/dev/kmem_mapper", O_RDWR | O_SYNC)) < 0) {
        perror("open");
        exit(-1);
    }

    /*
     * map device memory yo userspace
     */ 
    mmapped_device_memory = mmap(0, 
            kmem_mapped_area_len, 
            PROT_READ | PROT_WRITE, 
            MAP_SHARED | MAP_LOCKED, 
            kmem_mapper_fd, 
            kmem_mapped_area_len);
    if (mmapped_device_memory == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    /*fprintf(stderr, "Mmap address: 0x%lx\n", (unsigned long)mmapped_device_memory);*/
#endif


    TIMER_CLEAR(&barrier_wakeup_timer);
    TIMER_CLEAR(&barrier_call_timer);
    TIMER_CLEAR(&work_timer);

    pthread_create(&thr[0], NULL, heavy, (void *)&targs[0]);
    pthread_create(&thr[1], NULL, light, (void *)&targs[1]);
    pthread_join(thr[0], NULL);
    pthread_join(thr[1], NULL);

    /*fprintf(stdout, "Barrier wakeup time: tics: %lf , time=: %lf seconds", wakeup_tics, wakeup_tics/hz);*/
    /*fprintf(stdout, " Work time: tics: %lf , time=: %lf seconds\n", work_tics, work_tics/hz);*/
    fprintf(stdout, "Work_time: %lf     Wakeup_tics: %lf    Call_tics: %lf\n", work_tics/hz, wakeup_tics, call_tics);

#ifdef MWMON
    /*
     * unmap shared memory area
     */ 
    if(munmap(mmapped_device_memory, kmem_mapped_area_len) < 0) {
        perror("mmap");
        exit(-1);
    }
    /*
     * close kmem_mapper file
     */ 
    close(kmem_mapper_fd);
#endif


#ifdef SPINHALT 
    /* close cpuctrl file */ 
    close(cpuctrl_fd);
#endif


    return(0);
}
