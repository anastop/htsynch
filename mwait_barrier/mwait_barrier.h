#ifndef __MWAIT_BARRIER_H_
#define __MWAIT_BARRIER_H_

#include "synch.h"

/*
 * mmapped memory region size 
 */ 
#define MWAIT_NPAGES 1

extern char *mmapped_device_memory;
extern char *mmapped_device_memory2;
extern int kmem_mapper_fd, kmem_mapped_area_len;

typedef struct mwait_barrier_s {

    spin_t lock;    /* internal mutex */

    unsigned int left; /* current barrier count, # of threads still needed */
    unsigned int gathered;

    unsigned int init_count; /* number of threads needed for the barrier to continue */

    unsigned int curr_event; /* generation count: the actual futex. Its value changes
                                whenever the last thread arrives and a new round begins.*/
} mwait_barrier_t;

extern void mwait_barrier(mwait_barrier_t *barrier);
extern void mwait_barrier2(mwait_barrier_t *barrier);
extern void mwait_barrier_init(mwait_barrier_t *barrier, int nthreads);

#endif

