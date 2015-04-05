#ifndef __CPU_CONTROL_SYSCALLS_H
#define __CPU_CONTROL_SYSCALLS_H

/*
 * defined in kernel headers
 */ 
#define MWMON_MONITORED_MEM_BYTES       64      //apo thn CPUID (input: eax=0x05)
#define MWMON_ALIGN_BOUNDARY            64
#define MWMON_ORIGINAL_VAL      'a'
#define MWMON_NOTIFIED_VAL      'b'

#if defined KERNEL_2_6_13_1
#define MWMON_LINT_VAR_OFFSET   128
#endif


#if defined KERNEL_2_6_24_2
extern int  cpu_halt(void);
extern int  mwmon_mmap_sleep(void);
extern int  mwmon_mmap_sleep2(void);
#elif defined KERNEL_2_6_13_1
extern int  cpu_halt(void);
extern int  mwmon_kernel_init(void);
extern int  mwmon_kernel_sleep(void);
extern int  mwmon_kernel_notify(void);
extern int  mwmon_kernel_finalize(void);
extern int  mwmon_user_sleep(char *mwmon_memory);
extern int  mwmon_mmap_sleep(void);
extern int  mwmon_mmap_spin_while_greater(long int local_condition);
#endif



/*
 * Fast versions
 */
#if defined KERNEL_2_6_24_2
extern void fast_cpu_halt(void);
extern void fast_mwmon_mmap_sleep(void);
extern void fast_mwmon_mmap_sleep2(void);
#elif defined KERNEL_2_6_13_1
extern void fast_cpu_halt(void);
extern void fast_mwmon_kernel_init(void);
extern void fast_mwmon_kernel_sleep(void);
extern void fast_mwmon_kernel_notify(void);
extern void fast_mwmon_kernel_finalize(void);
extern void fast_mwmon_user_sleep(char *mwmon_memory);
extern void fast_mwmon_mmap_sleep(void);
extern void fast_mwmon_mmap_spin_while_greater(long int local_condition);
#endif

#endif
