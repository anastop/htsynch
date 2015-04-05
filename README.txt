1) "kernel_monitor_kernel_notification"
monitored memory allocated at kernel space (thru syscall),
waiting+monitoring occurs at kernel space (thru syscall),
notifitcation occurs at kernel space (thru system call)

2) "user_monitor_user_notification"
monitored memory allocated at user space, 
waiting+monitoring occurs at kernel space (thru syscall+copy
operations from userspace),
notification occurs at user space

3) "kernel_monitor_user_notification_with_mmap" + "kmem_mapper_module"
monitored memory allocated at kernel space (thru
"kmem_mapper" device driver),
waiting+monitoring occurs at kernel space (thru system call),
notification occurs at user space


In 2.6.13.1 the following system calls are implemented:
cpu_halt: HLT's the calling cpu

mwmon_kernel_init, 
mwmon_kernel_sleep, 
mwmon_kernel_notify,
mwmon_kernel_finalize: for case "1" 

mwmon_user_sleep: for case "2" 

mwmon_mmap_sleep,
mwmon_mmap_spin_while_greater: for case "3" 

Also, the following symbols are exported:
char* mwmon_mmap_area;
long int* mwmon_lint_var_addr;

In 2.6.24.2 kernel (the "twopackages" case), only the
following system calls are implemented:

cpu_halt,
mwmon_mmap_sleep,
mwmon_mmap_sleep2: the latter is used to do MON/MW on the
2nd physical package of a HT system (i.e., we assume that
mwmon_mmap_sleep, mwmon_mmap_sleep2 can be executed
simultaneously by 2 threads, as long as they do so on
different physical packages, and they are associated with
different monitored memory regions.  For this purpose, the
following 2 pointers are exported, one for each memory
region: 
char* mwmon_mmap_area;
char* mwmon_mmap_area2;

