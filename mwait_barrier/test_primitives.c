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

#include "cpu_control_syscalls.h"

/* To send wakeup IPIs via ioctl calls */
#include "cpuctrl.h"    

static _syscall0(pid_t, gettid)

double hz;
extern cpus_t cpus;
typedef struct targs_st {
    my_cpuset_t cpuset;
} targs_t;

mytimer_t work_timer, wakeup_timer, call_timer;
double wakeup_tics, work_tics, call_tics;
int iters, work_amount, sync_period;

#if defined(SPINPAUSE) || defined(SPINHALT)
/* Spinning with pause or halt related stuff... */
#define ORIGINAL_VALUE 0
#define NOTIFIED_VALUE 1
spin_t spin_var;
int cpuctrl_fd;
#endif

#ifdef MWMON
/* Mwait/monitor related stuff... */
#define MWAIT_NPAGES 1
char *mmapped_device_memory;
int kmem_mapper_fd, kmem_mapped_area_len;
#endif

#ifdef PTHREAD
pthread_cond_t cv;
pthread_mutex_t cv_mutex;
#endif

pthread_barrier_t pbarrier;

#define N 512
unsigned long ops=0, memops=0;

float A[N][N],B[N][N],C[N][N];
void do_work(unsigned int c)
{ 
    int i,j,k;

    for(i=0; i<c; i++)
        for(j=0; j<N; j++) {
            A[i][j] = 1.45;
            B[i][j] = 3.4;
            C[i][j] = 4.5;
            memops+=3;
        }

    for(i=0; i<c; i++)
        for(j=0; j<N; j++)
            for(k=0; k<N; k++) {
                A[i][j] += B[i][k]*C[k][j];
                ops += 2;
                memops += 3;
            }
}


//Heavyweight thread (e.g. worker)
void* heavy(void *arg) 
{
    targs_t *targs = (targs_t*)arg;
    int i,j,k;

    my_sched_setaffinity(gettid(), sizeof(targs->cpuset), &targs->cpuset);

    pthread_barrier_wait(&pbarrier);

    TIMER_START(&work_timer);


    for(i=0; i<N; i++) {

        for(j=0; j<N; j++) {
            for(k=0; k<N; k++) {
                A[i][j] += B[i][k]*C[k][j];
                ops += 2;
                memops += 3;
            }
        }

        if (!(i%sync_period)) {
            /* 
             * Notify sleeper 
             */
            /* To wakeup time tha einai to idio anexarthta apo th 
             * syxnothta sygxronismou (i.e. work_amount), opote arkei 
             * na to metrhsoume gia 1 timh tou work_amount, p.x. work_amount=512 
             */ 
            TIMER_START(&wakeup_timer);
            TIMER_START(&call_timer);
#if defined(MWMON)
            /* mwait */
            *mmapped_device_memory = MWMON_NOTIFIED_VAL;  

#elif defined(SPINPAUSE)
            /* spin loops w/ pause */
            spin_var = NOTIFIED_VALUE;

#elif defined(SPINHALT)
            /* spin loops w/ halt */
            spin_var = NOTIFIED_VALUE;
            ioctl(cpuctrl_fd, CPUCTRL_IOC_WAKEUP, 0); 

#elif defined(PTHREAD)
            /* pthread condition variables */
            pthread_mutex_lock(&cv_mutex);
            pthread_cond_signal(&cv);
            pthread_mutex_unlock(&cv_mutex);
#endif
            TIMER_STOP(&call_timer);
            call_tics = TIMER_TOTAL(&call_timer);
        }

    }

    TIMER_STOP(&work_timer);
    work_tics = TIMER_TOTAL(&work_timer);
    fprintf(stderr, "Heavy ended!\n");
    
    return NULL;
}
 
//Lightweight thread (e.g. prefetcher)
void* light(void *arg) 
{
    targs_t *targs = (targs_t*)arg;
    int i,j,k;
    
    my_sched_setaffinity(gettid(), sizeof(targs->cpuset), &targs->cpuset);

    pthread_barrier_wait(&pbarrier);

    /*for(i=0; i<iters; i++) {*/
        /*do_work(1);*/

    for(i=0; i<N; i++) {

        if(!(i%sync_period)) {
            /*
             * Sleep
             */
#if defined(MWMON)
            /* mwait */
            fast_mwmon_mmap_sleep();

#elif defined(SPINPAUSE)
            /* spin loops w/ pause */
            spin_on_condition(&spin_var, NOTIFIED_VALUE);
            spin_var = ORIGINAL_VALUE;
            
#elif defined(SPINHALT)
            /* spin loops w/ halt */
            spin_on_condition_cpuhalt(&spin_var, NOTIFIED_VALUE);
            spin_var = ORIGINAL_VALUE;

#elif defined(PTHREAD)
            /* pthread condition variables */
            pthread_mutex_lock(&cv_mutex);
            pthread_cond_wait(&cv, &cv_mutex);
            pthread_mutex_unlock(&cv_mutex);
#endif
            TIMER_STOP(&wakeup_timer);
            wakeup_tics = TIMER_TOTAL(&wakeup_timer);
        }

    }

    fprintf(stderr, "Light ended!\n");

    return NULL;
} 
 

int main(int argc, char *argv[]) {
    
    unsigned long nthreads=2;
    pthread_t *thr;
    targs_t *targs;
    int i,j;

    hz = read_hz();

    targs = (targs_t*)malloc(nthreads*sizeof(targs_t));
    thr = (pthread_t*)malloc(nthreads*sizeof(pthread_t));

    for(i=0; i<nthreads; i++) {
        MY_CPU_ZERO(&targs[i].cpuset);
    }
    pthread_barrier_init(&pbarrier, NULL, nthreads);
    MY_CPU_SET(cpus.pk[0].sib[0].cpu_number, &targs[0].cpuset);
    MY_CPU_SET(cpus.pk[0].sib[1].cpu_number, &targs[1].cpuset);

    sync_period = atoi(argv[1]);

    for(i=0; i<N; i++)
        for(j=0; j<N; j++) {
            A[i][j] = 1.45;
            B[i][j] = 3.4;
            C[i][j] = 4.5;
        }

    /*iters = atoi(argv[1]);*/
    /*work_amount = atoi(argv[2]);*/

#ifdef MWMON
    /*
     * kmem_mapper actions
     */
    /* open device */ 
    kmem_mapped_area_len = MWAIT_NPAGES * getpagesize();
    if ((kmem_mapper_fd = open("/dev/kmem_mapper", O_RDWR | O_SYNC)) < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    /* map device memory yo userspace */ 
    mmapped_device_memory = mmap(0, 
            kmem_mapped_area_len, 
            PROT_READ | PROT_WRITE, 
            MAP_SHARED | MAP_LOCKED, 
            kmem_mapper_fd, 
            kmem_mapped_area_len);
    if (mmapped_device_memory == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    /*fprintf(stderr, "Mmap address: 0x%lx\n", (unsigned long)mmapped_device_memory);*/
#endif


#ifdef SPINHALT
    /*
     * set cpu to wake up (after being HALTed) via IPIs
     */
    if((cpuctrl_fd = open("/dev/cpuctrl", O_RDONLY)) < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    int cpu_to_wakeup = cpus.pk[0].sib[1].cpu_number;
    if( ioctl(cpuctrl_fd, CPUCTRL_IOC_SET_CPU_TO_WAKEUP, &cpu_to_wakeup) == -1) {
        perror("Error in setting cpu to wakeup");
        exit(EXIT_FAILURE);
    }
#endif

#if defined(SPINHALT) || defined(SPINPAUSE)
    spin_var = ORIGINAL_VALUE;
#endif

#ifdef PTHREAD
    pthread_cond_init(&cv, NULL);
    pthread_mutex_init(&cv_mutex, NULL);
#endif


    TIMER_CLEAR(&work_timer);
    TIMER_CLEAR(&wakeup_timer);
    TIMER_CLEAR(&call_timer);

    pthread_create(&thr[0], NULL, heavy, (void *)&targs[0]);
    pthread_create(&thr[1], NULL, light, (void *)&targs[1]);
    pthread_join(thr[0], NULL);
    pthread_join(thr[1], NULL);

    /*fprintf(stdout, " Work time: tics: %lf , time=: %lf seconds\n", work_tics, work_tics/hz);*/
    /*fprintf(stdout, "%lf  ", work_tics/hz);*/
    /*fprintf(stdout, " Wakeup time: tics: %lf , time=: %lf seconds\n", wakeup_tics, wakeup_tics/hz);*/
    /*fprintf(stdout, " %lf \n", wakeup_tics);*/
    fprintf(stdout, "Work_time: %lf     Wakeup_tics: %lf    Call_tics: %lf\n", work_tics/hz, wakeup_tics, call_tics);

    /*fprintf(stdout, "Memops: %ld - ops: %ld\n", memops, ops);*/
#ifdef MWMON
    /* unmap shared memory area */ 
    if(munmap(mmapped_device_memory, kmem_mapped_area_len) < 0) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    /* close kmem_mapper file */ 
    close(kmem_mapper_fd);
#endif

#ifdef SPINHALT 
    /* close cpuctrl file */ 
    close(cpuctrl_fd);
#endif

    return(0);
}
