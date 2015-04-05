#ifndef __SPIN_H_
#define __SPIN_H_

#include <asm/unistd.h>

/**************************** spin-locks *****************************************/

#define LOCK_PREFIX "lock;"
typedef volatile long spin_t;

#define spin_init spin_unlock
#define spin_init_v2 spin_unlock_v2

#if defined NOP
#   define SPIN_INSTR   "rep; nop"
#elif defined PAUSE
#   define SPIN_INSTR   "pause"
#endif

#define SPIN_LOCK(spin_var) { \
    __asm__ __volatile__ ("\n" \
            "1:\tmovl $1, %%eax\n\t"    \
            "xchgl %0,%%eax\n\t"            \
            "cmpl $0,%%eax\n\t"         \
            "jne 2f\n\t"                \
            "jmp 3f\n"                  \
            "2:\t" SPIN_INSTR "\n\t"    \
            "cmpl $0,%0\n\t"        \
            "jne 2b\n\t"        \
            "jmp 1b\n"          \
            "3:\t\n"            \
            : "=m" (spin_var)   \
            : "m" (spin_var)    \
            : "%eax","memory"   \
            );                  \
}

#define SPIN_UNLOCK(spin_var) { \
    __asm__ __volatile__("movl $0, %0" \
    :"=m"(spin_var)); \
}


/********************************* barriers ************************************/
struct spin_barrier_s {
    spin_t lock;
    spin_t release_flag;
    int current_count;
    int total;
};
typedef struct spin_barrier_s spin_barrier_t;




//To system call number tou ioctl gia arch=x86_64 einai to 16. H ioctl pairnei 
//3 orismata ta opoia apothikeyontai pros anagnwsh stous kataxwrhtes edi, esi, edx.
//To apotelesma tou system call apothikeyetai pali ston eax (opote den mporoume
//na arxikopoihsoume mia fora ton eax kai na kaloume synexeia thn entolh syscall
//mesa sto loop - oi kataxwrhtes edi, esi, edx den allazoun kata th diarkeia ekteleshs
//tou macro, opote mporoun na mpoun stous input operands)
//Epilegoume rhta h metavlhth conditional_val na apothikeytei ston ebx.
//An kai to apotelesma ths klhsh tou system call apothikeyetai ston eax, den to xrhsimopoioume
//giati den mas endiaferei (ioctl). (Kanonika tha eprepe na kanoume error check klp, opws 
//ginetai sta antistoixa macros ston source ston kernel).

//arg1_ioctl (ston edi=D): o file descriptor 
//arg2_ioctl (ston esi=S): command (gia to halt pou theloume emeis, einai h CPUCTRL_HALT)
//arg3_ioctl (ston edx=d): command argument (0)

#define __syscall_clobber "r11","rcx","memory"
#define _NR_ioctl_x8664 16

#define FAST_IOCTL(arg1_ioctl, arg2_ioctl, arg3_ioctl) { \
    __asm__ __volatile__ ("\n"  \
            "movq $16, %%rax\n" \
            "syscall\n"         \
            : \
            : "D"((long)arg1_ioctl), "S"((long)arg2_ioctl), "d"((long)arg3_ioctl) \
            : __syscall_clobber \
            );  \
}

#define SPIN_ON_CONDITION_HLTCNT(spin_var, condition_val, cnt, arg1_ioctl, arg2_ioctl, arg3_ioctl)  { \
    __asm__ __volatile__ ("\n"  \
            "cmpl %1, %0\n"     \
            "je 2f\n"           \
            "1:\tpause\n\t"     \
            "movq $16, %%rax\n\t"   \
            "syscall\n\t"               \
            "incq %2\n\t"   \
            "cmpl %1, %0\n\t"   \
            "jne 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ib"(condition_val) , "m"(cnt), "D"((long)arg1_ioctl), "S"((long)arg2_ioctl), "d"((long)arg3_ioctl) \
            : __syscall_clobber \
            );  \
}

#define SPIN_BARRIER_LSENSE_HLTCNT(sb, lsense, cnt, arg1_ioctl, arg2_ioctl, arg3_ioctl) { \
    lsense = ~(lsense);         \
                                \
    SPIN_LOCK(sb.lock);         \
    (sb.current_count)++;       \
    if((sb.current_count) == (sb.total)) {  \
        (sb.current_count) = 0;             \
        (sb.release_flag) = (lsense);       \
    }                                       \
    SPIN_UNLOCK(sb.lock);                   \
    SPIN_ON_CONDITION_HLTCNT(sb.release_flag, lsense, cnt, arg1_ioctl, arg2_ioctl, arg3_ioctl); \
}


#define SPIN_ON_CONDITION_HLT(spin_var, condition_val, arg1_ioctl, arg2_ioctl, arg3_ioctl)  { \
    __asm__ __volatile__ ("\n"  \
            "cmpl %1, %0\n"     \
            "je 2f\n"           \
            "1:\tpause\n\t"     \
            "movq $16, %%rax\n\t"   \
            "syscall\n\t"               \
            "cmpl %1, %0\n\t"   \
            "jne 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ib"(condition_val) , "D"((long)arg1_ioctl), "S"((long)arg2_ioctl), "d"((long)arg3_ioctl)   \
            : __syscall_clobber \
            );  \
}

#define SPIN_BARRIER_LSENSE_HLT(sb, lsense, arg1_ioctl, arg2_ioctl, arg3_ioctl) { \
    lsense = ~(lsense);         \
                                \
    SPIN_LOCK(sb.lock);         \
    (sb.current_count)++;       \
    if((sb.current_count) == (sb.total)) {  \
        (sb.current_count) = 0;             \
        (sb.release_flag) = (lsense);       \
    }                                       \
    SPIN_UNLOCK(sb.lock);                   \
    SPIN_ON_CONDITION_HLT(sb.release_flag, lsense, arg1_ioctl, arg2_ioctl, arg3_ioctl); \
}





#define SPIN_ON_CONDITION(spin_var, condition_val)  { \
    __asm__ __volatile__ ("\n"  \
            "cmpl %1, %0\n"     \
            "je 2f\n"           \
            "1:\tpause\n\t"     \
            "cmpl %1, %0\n\t"   \
            "jne 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ir"(condition_val)     \
            );  \
}

//if ( spin_var >= condition_val ) { exit; }  else { spin; }
#define SPIN_ON_CONDITION_INEQ(spin_var, condition_val )    { \
    __asm__ __volatile__ ("\n"  \
            "cmpq %1, %0\n"     \
            "jge 2f\n"          \
            "1:\tpause\n\t"     \
            "cmpq %1, %0\n\t"   \
            "jle 1b\n"          \
            "2:\t\n"            \
            : :"m"(spin_var), "ib"(condition_val) ); }

//if ( spin_var >= condition_val ) { exit; }  else { spin; }
#define SPIN_ON_CONDITION_INEQ_CNT(spin_var, condition_val, cnt )   { \
    __asm__ __volatile__ ("\n"  \
            "cmpq %1, %0\n"     \
            "jge 2f\n"          \
            "1:\tpause\n\t"     \
            "incq %2\n\t"   \
            "cmpq %1, %0\n\t"   \
            "jle 1b\n"          \
            "2:\t\n"            \
            : :"m"(spin_var), "ib"(condition_val), "m"(cnt) ); }


#define IOCTL_HLT( arg1_ioctl, arg2_ioctl, arg3_ioctl ) { \
    __asm__ __volatile__ ("\n"  \
            "movq $16, %%rax\n" \
            "syscall\n"         \
            :                   \
            : "D"((long)arg1_ioctl), "S"((long)arg2_ioctl), "d"((long)arg3_ioctl)   \
            : __syscall_clobber \
            );  \
}


//here we use cpu_halt() custom syscall (number: 256) instead of ioctl implementation
//if ( spin_var >= condition_val ) { exit; }  else { spin; }
#define SPIN_ON_CONDITION_INEQ_CPUHALT(spin_var, condition_val )    { \
    __asm__ __volatile__ ("\n"  \
            "cmpq %1, %0\n"     \
            "jge 2f\n"          \
            "1:\tpause\n\t"     \
            "movq $256, %%rax\n\t"  \
            "syscall\n\t"               \
            "cmpq %1, %0\n\t"   \
            "jle 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ib"(condition_val)     \
            : __syscall_clobber \
            );  \
}

#define SPIN_ON_CONDITION_INEQ_CPUHALT_CNT(spin_var, condition_val, cnt )   { \
    __asm__ __volatile__ ("\n"  \
            "cmpq %1, %0\n"     \
            "jge 2f\n"          \
            "1:\tpause\n\t"     \
            "movq $256, %%rax\n\t"  \
            "syscall\n\t"               \
            "incq %2\n\t"   \
            "cmpq %1, %0\n\t"   \
            "jle 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ib"(condition_val), "m"(cnt)   \
            : __syscall_clobber \
            );  \
}

//if ( spin_var >= condition_val ) { exit; }  else { spin; }
#define SPIN_ON_CONDITION_INEQ_HLT(spin_var, condition_val, \
        arg1_ioctl, arg2_ioctl, arg3_ioctl )    { \
    __asm__ __volatile__ ("\n"  \
            "cmpq %1, %0\n"     \
            "jge 2f\n"          \
            "1:\tpause\n\t"     \
            "movq $16, %%rax\n\t"   \
            "syscall\n\t"               \
            "cmpq %1, %0\n\t"   \
            "jle 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ib"(condition_val) , "D"((long)arg1_ioctl), "S"((long)arg2_ioctl), "d"((long)arg3_ioctl)   \
            : __syscall_clobber \
            );  \
}


//if ( spin_var >= condition_val ) { exit; }  else { spin; }
#define SPIN_ON_CONDITION_INEQ_HLT_CNT(spin_var, condition_val, cnt, \
        arg1_ioctl, arg2_ioctl, arg3_ioctl )    { \
    __asm__ __volatile__ ("\n"  \
            "cmpq %1, %0\n"     \
            "jge 2f\n"          \
            "1:\tpause\n\t"     \
            "movq $16, %%rax\n\t"   \
            "syscall\n\t"               \
            "incq %2\n\t"   \
            "cmpq %1, %0\n\t"   \
            "jle 1b\n"          \
            "2:\t\n"            \
            :                   \
            :"m"(spin_var), "ib"(condition_val) , "m"(cnt), "D"((long)arg1_ioctl), "S"((long)arg2_ioctl), "d"((long)arg3_ioctl) \
            : __syscall_clobber \
            );  \
}





#define SPIN_BARRIER_LSENSE(sb, lsense) { \
    /*fprintf(stderr, "------Line %d file %s------\n", __LINE__, __FILE__);*/ \
    lsense = ~(lsense);         \
                                \
    SPIN_LOCK(sb.lock);         \
    (sb.current_count)++;       \
    if((sb.current_count) == (sb.total)) {  \
        (sb.current_count) = 0;             \
        (sb.release_flag) = (lsense);       \
    }                                       \
    SPIN_UNLOCK(sb.lock);                   \
    SPIN_ON_CONDITION(sb.release_flag, lsense); \
}

extern void spin_lock(spin_t *spin_var); 
extern void spin_unlock(spin_t *spin_var);
extern void spin_lock_v2(spin_t *spin_var);
extern void spin_unlock_v2(spin_t *spin_var);
extern void spin_on_condition(spin_t *spin_var, const int condition_val);
extern void spin_barrier_init(spin_barrier_t *sb, int total);
extern void spin_barrier_init_lsense(int *lsense);
extern void spin_barrier(spin_barrier_t *sb);
extern void spin_barrier_lsense(spin_barrier_t *sb, int *lsense);

#endif
