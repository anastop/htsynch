#include "spin.h"

/**************************** spin-locks *****************************************/

/*
 *  A=0: released lock
 *  A=1: want to get lock
 * 
 *  get_lock:   mov eax,1
 *              xchg eax, A     ;try to get lock - vale to 1 sthn A
 *              cmp eax, 0      ;test if successful (exei ginei released to lock? 
 *                              ;dhaldh, exei grapsei kapoios 0 sthn sync_var A?)
 *              jne spin_loop
 *
 *  critical_section:
 *              <critical section code>
 *              mov A, 0        ;release lock
 *              jmp continue    
 *
 *  spin_loop:  pause           ;short delay
 *              cmp 0, A        ;check if lock is free
 *              jne spin_loop   
 *              jmp get_lock
 *
 *  continue:
 *
 *
 *
 * Pseudo:
 *  - These A=1
 *  - H prohgoumenh timh tou A htan 0? Dhladh eixe ginei released to lock?
 *      An nai, ektelese to critical section.
 *      An oxi, kane spin loop:
 *          Tsekare diarkws an exei graftei to 0 sthn A, dhladh an exei ginei 
 *          released to lock.
 *          
 */


/*
 *  All threads use a swap instruction that atomically _reads the old value of 
 *  spin_var and stores a 1 to the spin_var_. The single winner will see the 0, 
 *  and the losers will see a 1 that was placed there by the winner (the losers 
 *  will continue to set the spin_var to 1 but that doesn't matter). When the 
 *  winner finishes with the critical section, it sets the spin_var to 0 to 
 *  release the lock.
 *
 */ 

#if 1
inline void spin_lock(spin_t *spin_var)
{
    __asm__ __volatile__ ("\n"          //Shmeiwsh: contention yparxei mono gia thn 
                                        //spin_var (global)
                                        //O eax fysika einai local gia kathe thread.
            "1:\tmovl $1, %%eax\n\t"            
            "xchgl %0,%%eax\n\t"      //Atomically set spin_var=1, 
                                      //    and read the previous value of spin_var.        
            "cmpl $0,%%eax\n\t"       //1st check (to avoid the 'pause' in case we 
                                      //have the opportunity to acquire the lock 
                                      //without the need to spin):
                                      // Is the lock free? (i.e. spin_var==0?)  
            "jne 2f\n\t"              //    If no, go to spin.  
            "jmp 3f\n"                //    If yes, then we have acquired the lock, 
                                      //    so we can enter into the critical section.
            
            "2:\t" SPIN_INSTR "\n\t"  //spin loop: continuously check for lock
            "cmpl $0,%0\n\t"          //Is the lock free? (i.e. spin_var==0?)
            "jne 2b\n\t"              //    If no, check over again.
            "jmp 1b\n"                //    If yes, go back to 1 to try to lock again.
                                      //    This time we must succeed.
            
            "3:\t\n"                  //Critical section entry point.
            : "=m" (*spin_var)
            : "m" (*spin_var)
            : "%eax","memory"   
            );
}

inline void spin_unlock(spin_t *spin_var)
{
    __asm__ __volatile__("movl $0, %0" \
                            :"=m"(*spin_var));
}
#endif
 
/*
inline void spin_lock_prof(spin_t *spin_var, unsigned long long *npauses)
{
    __asm__ __volatile__ ("\n"
            "1:\tmovl $1, %%eax\n\t"
            "xchgl %0,%%eax\n\t"
            "cmpl $0,%%eax\n\t"
            "jne 2f\n\t"
            "jmp 3f\n"
            "2:\t" SPIN_INSTR "\n\t"
            "incq %1\n\t"
            "cmpl $0,%0\n\t"
            "jne 2b\n\t"
            "jmp 1b\n"
            "3:\t\n"
            : "=m" (*spin_var), "=m"(*npauses)
            : "m" (*spin_var), "m"(*npauses)
            : "%eax","memory"   
            );
}*/


inline void spin_lock_v2(spin_t *spin_var)
{
    __asm__ __volatile__ ("\n"
            "1:\tlock; decl %0\n\t"     //atomically decrement
            "jns 3f\n"                  //if sign bit is clear,go to 3
            "2:\tcmpl $0,%0\n\t"        //spin - compare to 0           
            "pause\n\t"                 //spin-wait
            "jle 2b\n\t"                //spin - go to 2 if<=0 (locked)
            "jmp 1b\n"                  //unlocked; go back to 1 to try to lock again
            "3:\t\n"                    //we have acquired the lock
            :"=m" (*spin_var)
            :"m" (*spin_var)
            : "memory"
            );

}

inline void spin_unlock_v2(spin_t *spin_var)
{
    __asm__ __volatile__("movl $1, %0" \
                            :"=m"(*spin_var));
}

/********************************* barriers ************************************/
inline void spin_on_condition(spin_t *spin_var, const int condition_val)
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

inline void spin_barrier_init(spin_barrier_t *sb, int total)
{
    spin_init(&(sb->lock));
    sb->release_flag = 0;
    sb->current_count = 0;
    sb->total = total;
}

inline void spin_barrier_init_lsense(int *lsense)
{
    *lsense = 0;
}


/*  BARRIER (without local sense: einai dynaton na prokalesei deadlock metaxy 
 *  diadoxikwn klhsewn tou apo to idio thread, p.x. mesa se ena loop (vlepe 
 *  Hennesy Patterson)
 * 
 *  lock(counterlock);      //ensure update atomic
 *  if(count==0)
 *      release_flag=0;         //first to come=> reset release_flag
 *  count++;                //count the arrivals
 *  unlock(counterlock);    //release lock
 *  if(count==total) {      //all arived
 *      count=0;            //reset counter
 *      release_flag=1;         //release processes
 *  } else {
 *      spin(release_flag==1);  //wait for arrivals 
 *  }
 */
inline void spin_barrier(spin_barrier_t *sb)
{
    spin_lock(&(sb->lock));
    if(sb->current_count == 0) {
        //fprintf(stderr, "Thread %d before setting release_flag=0\n", gettid());
        sb->release_flag = 0;
    }
    (sb->current_count)++;
    //fprintf(stderr, "Thread %d increased current_count to %d\n", 
    //gettid(), sb->current_count);
    spin_unlock(&(sb->lock));

    if(sb->current_count == sb->total) {
        sb->current_count = 0;
        sb->release_flag = 1;
        //fprintf(stderr, "Thread %d set release_flag\n", gettid());
    } else {
        //fprintf(stderr, "Thread %d before spin_on_condition 
        //(current_count=%d, total=%d)\n", gettid(), sb->current_count, total);
        spin_on_condition(&(sb->release_flag), 1);
    }       
}   


/*  
 *  BARRIER (with local sense)
 *  
 */
inline void spin_barrier_lsense(spin_barrier_t *sb, int *lsense)
{
    *lsense = ~(*lsense);

    spin_lock(&(sb->lock));
    (sb->current_count)++;
    if(sb->current_count == sb->total) {
        sb->current_count = 0;
        sb->release_flag = *lsense;
    }
    spin_unlock(&(sb->lock));
    spin_on_condition(&(sb->release_flag), *lsense);
}


/****************************** conditionals **********************************/

/*  Arxika:
 *  produced = 0;
 * 
 *  PRODUCER:
 *      lock(var);
 *      while(produced==max_items)
 *          cond_wait(cond_var, lock);
 *          
 *      _PRODUCE()_
 *
 *      produced++
 *      cond_signal(cond_var);
 *
 *      unlock(var);
 *      
 *
 *  CONSUMER:
 *      lock(var);
 *      while(produced==0)
 *          cond_wait(cond_var, lock);
 *          
 *      _CONSUME()_
 *
 *      produced--
 *      cond_signal(cond_var);
 *
 *      unlock(var);
 *
 *
 */
   
