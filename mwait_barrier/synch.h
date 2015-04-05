#ifndef __SYNCH_H_
#define __SYNCH_H_

#include <asm/unistd.h>

/**************************** ATOMIC OPERATIONS *********************************/
static inline void atomic_inc(unsigned int *v)
{
    __asm__ __volatile__( "lock; incl %0"
                            :"=m" (*v)
                            :"m" (*v));
}


/*
 * Atomically decrements *v by 1 and returns true if the result is 0, 
 * or false for all other cases.
 */ 
static inline int atomic_dec_and_test(unsigned int *v)
{
    unsigned char c;

    __asm__ __volatile__( "lock; decl %0; sete %1"
                            :"=m" (*v), "=qm" (c)
                            :"m" (*v) 
                            : "memory");
    return c != 0;
}


/**************************** SPIN LOCKS  *****************************************/

typedef volatile unsigned int spin_t;

#define SPIN_LOCK_UNLOCKED  1 
#define SPIN_LOCK_LOCKED    0

/*
 * spin-lock 
 *
 */

static inline void spin_lock_init(spin_t *spin_var)
{
    *spin_var = SPIN_LOCK_UNLOCKED; 
}

static inline void spin_lock(spin_t *spin_var)
{
    __asm__ __volatile__( "\n"
        "1: lock; decl %0\n\t"  //Atomically decrease *spin_var
        "jns 2f\n"              //No sign flag set? Then *spin_var was unlocked (1). //Acquire it!!
        "3: pause\n\t"      //Sing flag set? Someone else has the lock, start spinning...
        "cmpl $0,%0\n\t"        //Did someone release the lock?
        "jle 3b\n\t"            //No.
        "jmp 1b\n"              //Yes? Go back to 1 to try to acquire the lock again... this time we ought to succeed...
        "2:\t"                  //Critical section entry point 
        : "=m" (*spin_var) 
        : "m"(*spin_var) 
        : "memory"); 

}

static inline void spin_unlock(spin_t *spin_var)
{
    __asm__ __volatile__("movl $1,%0" 
            :"=m" (*spin_var) 
            :
            : "memory");
}


/*
 * spin-lock "fast" (as implemented in pthreads)
 *
 */ 

#define spin_lock_init_fast     spin_lock_init
#define spin_unlock_fast        spin_unlock

static inline void spin_lock_fast(spin_t *spin_var)
{

    __asm__ __volatile__ ("\n"
        "1: lock; decl %0\n\t"
        "jne 2f\n\t"
        ".subsection 1\n\t"
        ".align 16\n"
        "2:\tpause\n\t"
        "cmpl $0, %0\n\t"
        "jg 1b\n\t"
        "jmp 2b\n\t"
        ".previous"
        : "=m" (*spin_var) 
        : "m" (*spin_var) 
        : "memory");
}


/*
 * spin-lock w/ compare and swap (cas) 
 *
 */

/*
 *  All threads use a swap instruction that atomically _reads the old value of 
 *  spin_var and stores a 0 to the spin_var_. The single winner will see the 1, 
 *  and the losers will see a 0 that was placed there by the winner (the losers 
 *  will continue to set the spin_var to 0 but that doesn't matter). When the 
 *  winner finishes with the critical section, it sets the spin_var to 1 to 
 *  release the lock.
 *
 */ 

#define spin_lock_init_cas  spin_lock_init
#define spin_unlock_cas     spin_unlock

static inline void spin_lock_cas(spin_t *spin_var)
{
    __asm__ __volatile__ ("\n"          
            "1:\tmovl $0, %%eax\n\t"            
            "xchgl %0,%%eax\n\t"      //Atomically set spin_var=0, and read the previous value of spin_var.     
            "cmpl $1,%%eax\n\t"       //1st check (to avoid the 'pause' in case we 
                                      //have the opportunity to acquire the lock 
                                      //without the need to spin):
                                      // Is the lock free? (i.e. spin_var==1?)  
            "jne 2f\n\t"              //    If no, go to spin.  
            "jmp 3f\n"                //    If yes, then we have acquired the lock! 
            
            "2: pause\n\t"            //spin loop: continuously check for lock
            "cmpl $1,%0\n\t"          //Is the lock free? (i.e. spin_var==1?)
            "jne 2b\n\t"              //    If no, check over again.
            "jmp 1b\n"                //    If yes, go back to 1 to try to lock again.
                                      //    This time we must succeed.
            
            "3:\t\n"                  //Critical section entry point.

            : "=m" (*spin_var) 
            : "m" (*spin_var)
            : "%eax","memory");
}

/********************************* SPIN LOOPS **********************************/
static inline void spin_on_condition(spin_t *spin_var, const int condition_val)
{
    __asm__ __volatile__ ("\n"
            "cmpl %1, %0\n"
            "je 2f\n"
            "1:\tpause\n\t"
            "cmpl %1, %0\n\t"
            "jne 1b\n"  
            "2:\t\n"
            :
            :"m"(*spin_var), "ir"(condition_val) 
            );

}

//if ( spin_var >= condition_val ) { exit; }  else { spin; }
static inline void spin_on_condition_ineq(spin_t *spin_var, const int condition_val)
{
    __asm__ __volatile__ ("\n"
            "cmpl %1, %0\n"
            "jge 2f\n"
            "1:\tpause\n\t"
            "cmpl %1, %0\n\t"
            "jle 1b\n"  
            "2:\t\n"
            :
            :"m"(*spin_var), "ir"(condition_val) 
            );

}



#define __syscall_clobber "r11","rcx","memory"

//here we use cpu_halt() custom syscall (number: 256) instead of ioctl implementation
static inline void spin_on_condition_cpuhalt(spin_t *spin_var, const int condition_val)
{
    __asm__ __volatile__ ("\n"
            "cmpl %1, %0\n"
            "je 2f\n"
            "1:\tpause\n\t"
            "movq $256, %%rax\n\t"  
            "syscall\n\t"               
            "cmpl %1, %0\n\t"
            "jne 1b\n"  
            "2:\t\n"
            :
            :"m"(*spin_var), "ir"(condition_val)
            : __syscall_clobber
            );

}



//if ( spin_var >= condition_val ) { exit; }  else { spin; }
static inline void spin_on_condition_ineq_cpuhalt(spin_t *spin_var, const int condition_val)
{
    __asm__ __volatile__ ("\n"
            "cmpl %1, %0\n"
            "jge 2f\n"
            "1:\tpause\n\t"
            "movq $256, %%rax\n\t"  
            "syscall\n\t"               
            "cmpl %1, %0\n\t"
            "jle 1b\n"  
            "2:\t\n"
            :
            :"m"(*spin_var), "ir"(condition_val)
            : __syscall_clobber
            );

}



/********************************* BARRIERS  **********************************/
typedef struct spin_barrier_s {
    spin_t lock;
    spin_t release_flag;
    int current_count;
    int nthreads;
} spin_barrier_t;

/*
 * with local sense (per-thread variable)
 *
 * example:
 *  spin_barrier_t gbarrier;    //global
 *  ...
 *  spin_barrier_init(&gbarrier, nthreads); //init global barrier
 *  for(i=0; i<nthreads; i++)               //init local sense
 *      spin_barrier_init_lsense(&targs[i].lsense);
 */ 
static inline void spin_barrier_init(spin_barrier_t *sb, int nthreads)
{
    spin_lock_init_fast(&(sb->lock));
    sb->release_flag = 1;
    sb->current_count = 0;
    sb->nthreads = nthreads;
}

//lsense: local var to each thread
static inline void spin_barrier_init_lsense(unsigned int *lsense)
{
    *lsense = 1;
}


#if 0
shared int count = P;
shared bool sense = true;
private bool local_sense = true;

void central_barrier() {
    local_sense = !local_sense;

    if (fetch_and_decrement(&count) == 1) {
        count = P;
        sense = local_sense;
    } else {
        while (sense != local_sense);
    }
}
#endif


static inline void spin_barrier_lsense(spin_barrier_t *sb, unsigned int *lsense)
{
    *lsense = !(*lsense);

    __asm__ __volatile__(
            "lock; incl %0" 
            :"=m" (sb->current_count)
            :"m" (sb->current_count));

    if(sb->current_count == sb->nthreads) {
        sb->current_count = 0;
        sb->release_flag = *lsense;
    } else
        spin_on_condition(&(sb->release_flag), *lsense);
}


static inline void spin_barrier_lsense_cpuhalt(spin_barrier_t *sb, unsigned int *lsense)
{
    *lsense = !(*lsense);

    __asm__ __volatile__(
            "lock; incl %0" 
            :"=m" (sb->current_count)
            :"m" (sb->current_count));

    if(sb->current_count == sb->nthreads) {
        sb->current_count = 0;
        sb->release_flag = *lsense;
    } else
        spin_on_condition_cpuhalt(&(sb->release_flag), *lsense);
}

#include "cpuctrl.h"
extern int cpuctrl_fd;
static inline void spin_barrier_lsense_cpuhalt_sendIPI(spin_barrier_t *sb, unsigned int *lsense)
{
    *lsense = !(*lsense);

    __asm__ __volatile__(
            "lock; incl %0" 
            :"=m" (sb->current_count)
            :"m" (sb->current_count));

    if(sb->current_count == sb->nthreads) {
        sb->current_count = 0;
        sb->release_flag = *lsense;
        ioctl(cpuctrl_fd, CPUCTRL_IOC_WAKEUP_ALLBUTSELF, 0); 
    } else
        spin_on_condition_cpuhalt(&(sb->release_flag), *lsense);
}



/*
 * without local sense:
 * einai dynaton na prokalesei deadlock metaxy diadoxikwn klhsewn 
 * tou apo to idio thread, p.x. mesa se ena loop (vlepe CA book)
 * 
 *  lock(counterlock);      //ensure update atomic
 *  if(count==0)
 *      release_flag=0;         //first to come=> reset release_flag
 *  count++;                //count the arrivals
 *  unlock(counterlock);    //release lock
 *  if(count==nthreads) {       //all arived
 *      count=0;            //reset counter
 *      release_flag=1;         //release processes
 *  } else {
 *      spin(release_flag==1);  //wait for arrivals 
 *  }
 */
static inline void spin_barrier(spin_barrier_t *sb)
{
    spin_lock_fast(&(sb->lock));
    if(sb->current_count == 0) {
        sb->release_flag = 0;
    }
    (sb->current_count)++;
    spin_unlock_fast(&(sb->lock));

    if(sb->current_count == sb->nthreads) {
        sb->current_count = 0;
        sb->release_flag = 1;
    } else {
        spin_on_condition(&(sb->release_flag), 1);
    }       
}   


#endif
