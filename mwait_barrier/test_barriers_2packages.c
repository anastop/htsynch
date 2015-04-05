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
    int id;
    my_cpuset_t cpuset;
    unsigned int lsense;
    int package;
} targs_t;

mytimer_t total_timer;
double wakeup_tics, total_tics, call_tics;
int heavy_work, light_work, iterations;

/* for initial synchronization AND testing of the Pthreads barrier */
pthread_barrier_t gbarrier;

#if defined(PTHREAD)
pthread_barrier_t pbarrier0, pbarrier1;
#endif

#if defined(SPINPAUSE) || defined(SPINHALT)
spin_barrier_t sbarrier0, sbarrier1;
#ifdef SPINHALT
int cpuctrl_fd;
#endif
#endif

#if defined(MWMON)
mwait_barrier_t mwbarrier0, mwbarrier1;
/* Distance (in bytes) between the 2 monitored memory regions */
#define MONITORED_REGIONS_DISTANCE 512
#endif

#define N 4
float **A0,**B0,**C0;
float **A1,**B1,**C1;
float **A2,**B2,**C2;
float **A3,**B3,**C3;

void allocate_matrices(void)
{
    int i;

    A0 = (float**)malloc(N*sizeof(float*));
    B0 = (float**)malloc(N*sizeof(float*));
    C0 = (float**)malloc(N*sizeof(float*));
    for(i=0; i<N; i++) {
        A0[i] = (float*)malloc(N*sizeof(float));
        B0[i] = (float*)malloc(N*sizeof(float));
        C0[i] = (float*)malloc(N*sizeof(float));
    }
    
    A1 = (float**)malloc(N*sizeof(float*));
    B1 = (float**)malloc(N*sizeof(float*));
    C1 = (float**)malloc(N*sizeof(float*));
    for(i=0; i<N; i++) {
        A1[i] = (float*)malloc(N*sizeof(float));
        B1[i] = (float*)malloc(N*sizeof(float));
        C1[i] = (float*)malloc(N*sizeof(float));
    }

    A2 = (float**)malloc(N*sizeof(float*));
    B2 = (float**)malloc(N*sizeof(float*));
    C2 = (float**)malloc(N*sizeof(float*));
    for(i=0; i<N; i++) {
        A2[i] = (float*)malloc(N*sizeof(float));
        B2[i] = (float*)malloc(N*sizeof(float));
        C2[i] = (float*)malloc(N*sizeof(float));
    }

    A3 = (float**)malloc(N*sizeof(float*));
    B3 = (float**)malloc(N*sizeof(float*));
    C3 = (float**)malloc(N*sizeof(float*));
    for(i=0; i<N; i++) {
        A3[i] = (float*)malloc(N*sizeof(float));
        B3[i] = (float*)malloc(N*sizeof(float));
        C3[i] = (float*)malloc(N*sizeof(float));
    }

} 

static void do_work(int c, float **A, float **B, float **C)
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


//Heavyweight thread (e.g. worker)
void* heavy(void *arg) 
{
    targs_t *targs = (targs_t*)arg;
    unsigned int *lsense = &targs->lsense;
    int i;
    int package=targs->package;
    float **A, **B, **C;
    mytimer_t work_timer;
    double work_tics;

#if defined(MWMON)
    mwait_barrier_t *my_mwbarrier;
    void (*my_mwait_barrier_func)(mwait_barrier_t*);
#elif defined(SPINPAUSE) || defined(SPINHALT)
    spin_barrier_t *my_sbarrier;
#elif defined(PTHREAD)
    pthread_barrier_t *my_pbarrier;
#endif
    my_sched_setaffinity(gettid(), sizeof(targs->cpuset), &targs->cpuset);


    if(package==0) { /*Heavy thread on first package*/
        A=A0; B=B0; C=C0;
#if defined(MWMON)
        my_mwbarrier = &mwbarrier0;
        my_mwait_barrier_func = mwait_barrier;
#elif defined(SPINPAUSE) || defined(SPINHALT)
        my_sbarrier = &sbarrier0;
#elif defined(PTHREAD)
        my_pbarrier = &pbarrier0;
#endif


    } else if(package==1) { /*Heavy thread on second package*/
        A=A2; B=B2; C=C2;
#if defined(MWMON)
        my_mwbarrier = &mwbarrier1;
        my_mwait_barrier_func = mwait_barrier2;
#elif defined(SPINPAUSE) || defined(SPINHALT)
        my_sbarrier = &sbarrier1;
#elif defined(PTHREAD)
        my_pbarrier = &pbarrier1;
#endif
    }

    pthread_barrier_wait(&gbarrier);
    
    /*fprintf(stderr, "Heavy before starting... (package %d)!\n", package);*/

    TIMER_CLEAR(&work_timer);
    TIMER_START(&work_timer);
    for(i=0; i<iterations; i++) { 

        do_work(heavy_work,A,B,C);
        /*fprintf(stderr, "Heavy ended hard work (package %d)!\n", package);*/
    
#if defined(MWMON)
        my_mwait_barrier_func(my_mwbarrier);
#elif defined(SPINPAUSE)
        spin_barrier_lsense(my_sbarrier, lsense);
#elif defined(SPINHALT)
        spin_barrier_lsense_cpuhalt_sendIPI(my_sbarrier, lsense);
#elif defined(PTHREAD)
        pthread_barrier_wait(my_pbarrier);
#endif
        /*fprintf(stderr,"Heavy%d: %d\n", package, i);*/
    }
    TIMER_STOP(&work_timer);
    work_tics = TIMER_TOTAL(&work_timer);
    /*fprintf(stdout, "Heavy work time: %lf\n", work_tics/hz);*/

    /*fprintf(stderr, "Heavy terminated (package %d)!\n", package);*/
    return NULL;
}
 
//Lightweight th read (e.g. prefetcher)
void* light(void *arg) 
{
    targs_t *targs = (targs_t*)arg;
    unsigned int *lsense = &targs->lsense;
    int i;
    int package=targs->package;
    float **A, **B, **C;
    
#if defined(MWMON)
    mwait_barrier_t *my_mwbarrier;
    void (*my_mwait_barrier_func)(mwait_barrier_t*);
#elif defined(SPINPAUSE) || defined(SPINHALT)
    spin_barrier_t *my_sbarrier;
#elif defined(PTHREAD)
    pthread_barrier_t *my_pbarrier;
#endif
    my_sched_setaffinity(gettid(), sizeof(targs->cpuset), &targs->cpuset);


    if(package==0) { /*Light thread on first package*/
        A=A1; B=B1; C=C1;
#if defined(MWMON)
        my_mwbarrier = &mwbarrier0;
        my_mwait_barrier_func = mwait_barrier;
#elif defined(SPINPAUSE) || defined(SPINHALT)
        my_sbarrier = &sbarrier0;
#elif defined(PTHREAD)
        my_pbarrier = &pbarrier0;
#endif


    } else if(package==1) { /*Light thread on second package*/
        A=A3; B=B3; C=C3;
#if defined(MWMON)
        my_mwbarrier = &mwbarrier1;
        my_mwait_barrier_func = mwait_barrier2;
#elif defined(SPINPAUSE) || defined(SPINHALT)
        my_sbarrier = &sbarrier1;
#elif defined(PTHREAD)
        my_pbarrier = &pbarrier1;
#endif
    }


    pthread_barrier_wait(&gbarrier);
    
    /*fprintf(stderr, "Light before starting... (package %d)!\n", package);*/

    for(i=0; i<iterations; i++) {
        do_work(light_work,A,B,C);
        /*fprintf(stderr, "Light ended short work (package %d)!\n", package);*/

#if defined(MWMON)
        my_mwait_barrier_func(my_mwbarrier);
#elif defined(SPINPAUSE)
        spin_barrier_lsense(my_sbarrier, lsense);
#elif defined(SPINHALT)
        spin_barrier_lsense_cpuhalt_sendIPI(my_sbarrier, lsense);
#elif defined(PTHREAD)
        pthread_barrier_wait(my_pbarrier);
#endif
        /*fprintf(stderr,"Light%d: %d\n", package, i);*/
    }

    /*fprintf(stderr, "Light terminated (package %d)!\n", package);*/
    return NULL;
} 
 

int main(int argc, char *argv[]) {
    
    unsigned long i,j, nthreads=4;
    pthread_t *thr;
    targs_t *targs;

    heavy_work = atoi(argv[1]);
    light_work = atoi(argv[2]);
    iterations = atoi(argv[3]);

    hz = read_hz();

    targs = (targs_t*)malloc(nthreads*sizeof(targs_t));
    thr = (pthread_t*)malloc(nthreads*sizeof(pthread_t));

    allocate_matrices();

    for(i=0; i<nthreads; i++) {
        targs[i].id = i;
        MY_CPU_ZERO(&targs[i].cpuset);
    }

    targs[0].package = 0;
    targs[1].package = 0;
    targs[2].package = 1;
    targs[3].package = 1;

    MY_CPU_SET(cpus.pk[0].sib[0].cpu_number, &targs[0].cpuset);
    MY_CPU_SET(cpus.pk[0].sib[1].cpu_number, &targs[1].cpuset);
    MY_CPU_SET(cpus.pk[1].sib[0].cpu_number, &targs[2].cpuset);
    MY_CPU_SET(cpus.pk[1].sib[1].cpu_number, &targs[3].cpuset);

    pthread_barrier_init(&gbarrier, NULL, 4);

#if defined(PTHREAD)
    pthread_barrier_init(&pbarrier0, NULL, 2);
    pthread_barrier_init(&pbarrier1, NULL, 2);
#endif

#if defined(SPINHALT) || defined(SPINPAUSE)
    spin_barrier_init(&sbarrier0, 2);
    spin_barrier_init(&sbarrier1, 2);

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
    mwait_barrier_init(&mwbarrier0, 2);
    mwait_barrier_init(&mwbarrier1, 2);

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
    mmapped_device_memory2 = mmapped_device_memory + MONITORED_REGIONS_DISTANCE;
    /*fprintf(stderr, "Mmap address: 0x%lx\n", (unsigned long)mmapped_device_memory);*/
    /*fprintf(stderr, "Second memory region: 0x%lx\n", (unsigned long)mmapped_device_memory2);*/
#endif


    TIMER_CLEAR(&total_timer);
    TIMER_START(&total_timer);

    pthread_create(&thr[0], NULL, heavy, (void *)&targs[0]);
    pthread_create(&thr[1], NULL, light, (void *)&targs[1]);
    pthread_create(&thr[2], NULL, heavy, (void *)&targs[2]);
    pthread_create(&thr[3], NULL, light, (void *)&targs[3]);
    pthread_join(thr[0], NULL);
    pthread_join(thr[1], NULL);
    pthread_join(thr[2], NULL);
    pthread_join(thr[3], NULL);
    TIMER_STOP(&total_timer);
    total_tics = TIMER_TOTAL(&total_timer);
    /*fprintf(stdout, "Barrier wakeup time: tics: %lf , time=: %lf seconds", wakeup_tics, wakeup_tics/hz);*/
    /*fprintf(stdout, " Work time: tics: %lf , time=: %lf seconds\n", total_tics, total_tics/hz);*/
    fprintf(stdout, "Work_time: %lf     Wakeup_tics: %lf    Call_tics: %lf\n", total_tics/hz, wakeup_tics, call_tics);

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
