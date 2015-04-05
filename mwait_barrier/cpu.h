#ifndef __CPU_H_
#define __CPU_H_

//CPUs configuration at xenons
#define MAX_PACKAGES 2
#define MAX_SIBLINGS 2

//Max number of logical processors (for various structures allocation)
#define MAXTHREADS  8

#if defined(CPU_AMD)
#define NUM_CPUS 4
#elif defined(CPU_XEON)
#define NUM_CPUS 4
#elif defined(CPU_CORE)
#define NUM_CPUS 4
#elif defined(CPU_CORESINGLE)
#define NUM_CPUS 2
#elif defined(CPU_P6)
#define NUM_CPUS 2
#else 
#error "no prfcnt implementation found"
#endif



typedef struct sibling_st {
    unsigned int cpu_number;
} sibling_t;

typedef struct package_st {
    sibling_t sib[MAX_SIBLINGS];
    int nsiblings;  //#siblings per package
    int id;     //package id
#ifdef NUMA
    int memnode_number;
#endif
} package_t;

typedef struct cpus_st {
    package_t pk[MAX_PACKAGES];
    int npackages;  //#packages
} cpus_t;

extern int get_pkid(int cpu_number, cpus_t *cpus);

/* Affinity issues */
#include <sys/types.h>
typedef unsigned long my_cpuset_t;
#define MY_CPU_SETSIZE (sizeof(my_cpuset_t)*8)
extern int my_sched_setaffinity(pid_t pid, size_t my_cpuset_size, my_cpuset_t *cpuset); 
extern int my_sched_getaffinity(pid_t pid, size_t my_cpuset_size, my_cpuset_t *cpuset); 
extern void MY_CPU_ZERO(my_cpuset_t *cpuset);
extern void MY_CPU_SET(int cpu_no, my_cpuset_t *cpuset);
extern int MY_CPU_ISSET(int cpu_no, my_cpuset_t *cpuset); 
#endif
