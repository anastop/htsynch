#include "syscall_macros.h"

#if defined KERNEL_2_6_13_1
    #define __NR_cpu_halt       256
    #define __NR_mwmon_kernel_init 257
    #define __NR_mwmon_kernel_sleep 258
    #define __NR_mwmon_kernel_notify 259
    #define __NR_mwmon_kernel_finalize 260
    #define __NR_mwmon_user_sleep 261
    #define __NR_mwmon_mmap_sleep 262
    #define __NR_mwmon_mmap_spin_while_greater 263
#elif defined KERNEL_2_6_24_2
    #define __NR_cpu_halt       286
    #define __NR_mwmon_mmap_sleep 287
    #define __NR_mwmon_mmap_sleep2 288
#endif


#if defined KERNEL_2_6_24_2
_syscall0(int, cpu_halt)
_syscall0(int, mwmon_mmap_sleep)
_syscall0(int, mwmon_mmap_sleep2)
#elif defined KERNEL_2_6_13_1
_syscall0(int, cpu_halt)
_syscall0(int, mwmon_mmap_sleep)
_syscall0(int, mwmon_kernel_init)
_syscall0(int, mwmon_kernel_sleep)
_syscall0(int, mwmon_kernel_notify)
_syscall0(int, mwmon_kernel_finalize)
_syscall1(int, mwmon_user_sleep, char*, mwmon_memory) 
_syscall1(int, mwmon_mmap_spin_while_greater, long int, local_condition)
#endif

/*
 * Fast versions
 */
//Used internally by _fast_syscallN 
long __dummy_res;

#if defined KERNEL_2_6_24_2
_fast_syscall0(void, cpu_halt)
_fast_syscall0(void, mwmon_mmap_sleep)
_fast_syscall0(void, mwmon_mmap_sleep2)
#elif defined KERNEL_2_6_13_1
_fast_syscall0(void, cpu_halt)
_fast_syscall0(void, mwmon_mmap_sleep)
_fast_syscall0(void, mwmon_kernel_init)
_fast_syscall0(void, mwmon_kernel_sleep)
_fast_syscall0(void, mwmon_kernel_notify)
_fast_syscall0(void, mwmon_kernel_finalize)
_fast_syscall1(void, mwmon_user_sleep, char*, mwmon_memory) 
_fast_syscall1(void, mwmon_mmap_spin_while_greater, long int, local_condition)
#endif
