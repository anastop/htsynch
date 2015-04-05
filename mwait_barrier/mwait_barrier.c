#include "mwait_barrier.h"
#include "cpu_control_syscalls.h"
#include "synch.h"

/*
 * Pointers to first and second memory region.
 * They respectively show to the same locations 
 * as pointers mwmon_mmap_area and mwmon_mmap_area2
 * in kernel space.
 */ 
char *mmapped_device_memory;
char *mmapped_device_memory2;

int kmem_mapper_fd, kmem_mapped_area_len;

/*
 * Ypotheseis gia to mwait_barrier kai periorismoi tou 
 * se sxesh me ta alla barriers:
 * - ypothetoume oti exoume 2 mono threads (waiter + worker) ana 
 *   barrier, 'h ana package, kathe ena apo ta opoia einai bound
 *   se diaforetika contexts tou idiou HT processor
 * - mporoun na yparxoun to poly 2 barrier metavlhtes, gia zeygh 
 *   threads pou ektelountai se diaforetika packages
 */

void mwait_barrier_init(mwait_barrier_t *barrier, int nthreads)
{
	barrier->left = nthreads;
	barrier->gathered = 0;
	barrier->init_count = nthreads;
	spin_lock_init_fast(&barrier->lock);
}

/*
 * Barrier for threads executing on first physical package
 */ 
void mwait_barrier(mwait_barrier_t *barrier)
{
	unsigned int init_count = barrier->init_count;

	/* Make sure we are alone.  */
	spin_lock_fast(&barrier->lock);

	/* One more arrival.  */
	/*--barrier->left;*/
	++barrier->gathered;

	/* Is this the last thread? */
	/*if (barrier->left == 0) {*/
	if (barrier->gathered == init_count) {
		/* 
		 * Yes. Increment the event counter to avoid invalid wake-ups and
		 * tell the current waiters that it is their turn.  
		 */
		
		/*++barrier->curr_event;*/
		
		/* Wake up waiter... */
		*mmapped_device_memory = MWMON_NOTIFIED_VAL;  
		
	} else {
		/* 
		 * The number of the event we are waiting for.  The barrier's event
		 * number must be bumped before we continue.  
		 */
		/*unsigned int event = barrier->curr_event;*/
		
		/* Before suspending, make the barrier available to others.  */
		spin_unlock_fast(&barrier->lock);
		
		/* Wait for the event counter of the barrier to change.  */
		fast_mwmon_mmap_sleep();
		/*do*/
			/*lll_futex_wait (&barrier->curr_event, event);*/
		/*while (event == barrier->curr_event);*/
	}
	
	/* Make sure the init_count is stored locally or in a register.  */
	/*init_count = barrier->init_count;*/

    /* If this was the last woken thread, unlock.  */
	/*if (atomic_inc(&barrier->left) == init_count)*/
	if (atomic_dec_and_test(&barrier->gathered))
		/* We are done.  */
		spin_unlock_fast(&barrier->lock);
	
}


/*
 * Barrier for threads executing on second physical package
 */ 
void mwait_barrier2(mwait_barrier_t *barrier)
{
	unsigned int init_count = barrier->init_count;

	/* Make sure we are alone.  */
	spin_lock_fast(&barrier->lock);

	/* One more arrival.  */
	/*--barrier->left;*/
	++barrier->gathered;

	/* Is this the last thread? */
	/*if (barrier->left == 0) {*/
	if (barrier->gathered == init_count) {
		/* 
		 * Yes. Increment the event counter to avoid invalid wake-ups and
		 * tell the current waiters that it is their turn.  
		 */
		
		/*++barrier->curr_event;*/
		
		/* Wake up waiter... */
		*mmapped_device_memory2 = MWMON_NOTIFIED_VAL;  
		
	} else {
		/* 
		 * The number of the event we are waiting for.  The barrier's event
		 * number must be bumped before we continue.  
		 */
		/*unsigned int event = barrier->curr_event;*/
		
		/* Before suspending, make the barrier available to others.  */
		spin_unlock_fast(&barrier->lock);
		
		/* Wait for the event counter of the barrier to change.  */
		fast_mwmon_mmap_sleep2();
		/*do*/
			/*lll_futex_wait (&barrier->curr_event, event);*/
		/*while (event == barrier->curr_event);*/
	}
	
	/* Make sure the init_count is stored locally or in a register.  */
	/*init_count = barrier->init_count;*/

    /* If this was the last woken thread, unlock.  */
	/*if (atomic_inc(&barrier->left) == init_count)*/
	if (atomic_dec_and_test(&barrier->gathered))
		/* We are done.  */
		spin_unlock_fast(&barrier->lock);
	
}
