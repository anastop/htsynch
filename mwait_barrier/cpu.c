#include "cpu.h"

#include <sys/types.h>
#include <linux/unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "syscall_macros.h"

_syscall3(int,sched_setaffinity,pid_t,pid,unsigned int,len,unsigned long *,maskptr)
_syscall3(int,sched_getaffinity,pid_t,pid,unsigned int,len,unsigned long *,maskptr)


#if defined(CPU_XEON)
/*
 * xenons configuration
 */ 
cpus_t cpus = {
    .npackages = 2,
    .pk = {

        //package1
        {
            .nsiblings = 2,
            .id = 0,
            .sib = {
                {.cpu_number=0},    //sib1
                {.cpu_number=2}     //sib2
            }
        },
        //package2  
        {
            .nsiblings = 2,
            .id = 1,
            .sib = {
                {.cpu_number=1},    //sib1
                {.cpu_number=3}     //sib2
            }
        }
    }
};
#elif defined(CPU_CORE)     
/*
 * jgora configuration
 */ 
cpus_t cpus = {
    .npackages = 2,
    .pk = {

        //package1
        {
            .nsiblings = 2,
            .id = 0,
            .sib = {
                {.cpu_number=0},    //sib1
                {.cpu_number=1}     //sib2
            }
        },
        //package2  
        {
            .nsiblings = 2,
            .id = 1,
            .sib = {
                {.cpu_number=2},    //sib1
                {.cpu_number=3}     //sib2
            }
        }
    }
};
#elif defined(CPU_CORESINGLE)       
/*
 * solar configuration
 */ 
cpus_t cpus = {
    .npackages = 1,
    .pk = {
        //package1
        {
            .nsiblings = 2,
            .id = 0,
            .sib = {
                {.cpu_number=0},    //sib1
                {.cpu_number=1}     //sib2
            }
        }
    }
};


#elif defined(CPU_AMD)
/*
 * opty configuration
 */ 
cpus_t cpus = {
    .npackages = 2,
    .pk = {

        //package1
        {
#ifdef NUMA
            .memnode_number = 0,
#endif
            .nsiblings = 2,
            .id = 0,
            .sib = {
                {.cpu_number=0},    //sib1
                {.cpu_number=1}     //sib2
            }
        },
        //package2  
        {
#ifdef NUMA
            .memnode_number = 1,
#endif
            .nsiblings = 2,
            .id = 1,
            .sib = {
                {.cpu_number=2},    //sib1
                {.cpu_number=3}     //sib2
            }
        }
    }
};

#elif defined(CPU_P6)
/*
 * zealots configuration
 */ 
cpus_t cpus = {
    .npackages = 2,
    .pk = {

        //package1
        {
            .nsiblings = 1,
            .id = 0,
            .sib = {
                {.cpu_number=0},    //sib1
            }
        },
        //package2  
        {
            .nsiblings = 1,
            .id = 1,
            .sib = {
                {.cpu_number=1},    //sib1
            }
        }
    }
};

#else
    #error "No cpu configuration found"
#endif

int get_pkid(int cpu_number, cpus_t *cpus)
{ 
    int i,j;

    for(i=0; i<cpus->npackages; i++)
        for(j=0; j<cpus->pk[i].nsiblings; j++)
            if(cpu_number == cpus->pk[i].sib[j].cpu_number)
                return cpus->pk[i].id;

    return -1;
}

inline int my_sched_setaffinity(pid_t pid, size_t cpuset_size, my_cpuset_t *cpuset)
{
    int res = sched_setaffinity(pid, (unsigned int)cpuset_size, (unsigned long*)cpuset);
    if(res==-1) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
    return res;
}

inline int my_sched_getaffinity(pid_t pid, size_t cpuset_size, my_cpuset_t *cpuset)
{
    int res = sched_getaffinity(pid, (unsigned int)cpuset_size, (unsigned long*)cpuset);
    if(res==-1) {
        perror("sched_getaffinity");
        exit(EXIT_FAILURE);
    }
    return res;
}

//Shmeiwsh: h arithmisi twn cpus arxizei apo to 0

inline void MY_CPU_ZERO(my_cpuset_t *cpuset)
{
    *cpuset = 0;
}

inline void MY_CPU_SET(int cpu_no, my_cpuset_t *cpuset)
{
    *cpuset |= 1<<cpu_no;
}

//return values:
//zero, an einai 'off' h sygkekrimenh cpu
//non-zero, an einai 'on'
inline int MY_CPU_ISSET(int cpu_no, my_cpuset_t *cpuset) 
{
    return ((unsigned long)1<<cpu_no)&(*cpuset);
}
