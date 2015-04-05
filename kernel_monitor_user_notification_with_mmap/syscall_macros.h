#ifndef __SYSCALL_MACROS_H
#define __SYSCALL_MACROS_H

#include <errno.h>

#if (CPU_BITS == 32)

/*
 * 'Fast', customized macros: ignore (improperly) return values 
 * (they actually don't return anything, i.e. 'type' must be 'void')
 */ 


#define _fast_syscall0(type,name) \
    type fast_##name(void) \
{ \
    __asm__ volatile ("int $0x80" \
                    : "=a" (__dummy_res) \
                    : "0" (__NR_##name)); \
}

#define _fast_syscall1(type,name,type1,arg1) \
    type fast_##name(type1 arg1) \
{ \
    __asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
                    : "=a" (__dummy_res) \
                    : "0" (__NR_##name),"ri" ((long)(arg1)) : "memory"); \
}



/*
 * Normal macros
 */ 
#define __syscall_return(type, res) \
    do { \
                if ((unsigned long)(res) >= (unsigned long)(-(128 + 1))) { \
                                    res = -1; \
                            } \
                return (type) (res); \
    } while (0)


/* XXX - _foo needs to be __foo, while __NR_bar could be _NR_bar. */
#define _syscall0(type,name) \
    type name(void) \
{ \
    long __res; \
    __asm__ volatile ("int $0x80" \
                    : "=a" (__res) \
                    : "0" (__NR_##name)); \
    __syscall_return(type,__res); \
}


#define _syscall1(type,name,type1,arg1) \
    type name(type1 arg1) \
{ \
    long __res; \
    __asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
                    : "=a" (__res) \
                    : "0" (__NR_##name),"ri" ((long)(arg1)) : "memory"); \
    __syscall_return(type,__res); \
}


#define _syscall2(type,name,type1,arg1,type2,arg2) \
    type name(type1 arg1,type2 arg2) \
{ \
    long __res; \
    __asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
                    : "=a" (__res) \
                    : "0" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)) \
                    : "memory"); \
    __syscall_return(type,__res); \
}


#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
    type name(type1 arg1,type2 arg2,type3 arg3) \
{ \
    long __res; \
    __asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
                    : "=a" (__res) \
                    : "0" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)), \
                              "d" ((long)(arg3)) : "memory"); \
    __syscall_return(type,__res); \
}


#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
    type name (type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
    long __res; \
    __asm__ volatile ("push %%ebx ; movl %2,%%ebx ; int $0x80 ; pop %%ebx" \
                    : "=a" (__res) \
                    : "0" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)), \
                      "d" ((long)(arg3)),"S" ((long)(arg4)) : "memory"); \
    __syscall_return(type,__res); \
}


#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
                  type5,arg5) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5) \
{ \
    long __res; \
    __asm__ volatile ("push %%ebx ; movl %2,%%ebx ; movl %1,%%eax ; " \
                              "int $0x80 ; pop %%ebx" \
                    : "=a" (__res) \
                    : "i" (__NR_##name),"ri" ((long)(arg1)),"c" ((long)(arg2)), \
                      "d" ((long)(arg3)),"S" ((long)(arg4)),"D" ((long)(arg5)) \
                    : "memory"); \
    __syscall_return(type,__res); \
}


#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
                  type5,arg5,type6,arg6) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6) \
{ \
    long __res; \
      struct { long __a1; long __a6; } __s = { (long)arg1, (long)arg6 }; \
    __asm__ volatile ("push %%ebp ; push %%ebx ; movl 4(%2),%%ebp ; " \
                              "movl 0(%2),%%ebx ; movl %1,%%eax ; int $0x80 ; " \
                              "pop %%ebx ;  pop %%ebp" \
                    : "=a" (__res) \
                    : "i" (__NR_##name),"0" ((long)(&__s)),"c" ((long)(arg2)), \
                      "d" ((long)(arg3)),"S" ((long)(arg4)),"D" ((long)(arg5)) \
                    : "memory"); \
    __syscall_return(type,__res); \
}



#elif (CPU_BITS == 64)

#define __syscall_clobber "r11","rcx","memory" 
#define __syscall "syscall"

/*
 * 'Fast', customized macros: ignore (improperly) return values 
 * (they actually don't return anything, i.e. 'type' must be 'void')
 */ 

#define _fast_syscall0(type,name) \
type fast_##name(void) \
{ \
__asm__ volatile (__syscall \
    : "=a" (__dummy_res) \
    : "0" (__NR_##name) : __syscall_clobber ); \
}


#define _fast_syscall1(type,name,type1,arg1) \
type fast_##name(type1 arg1) \
{ \
__asm__ volatile (__syscall \
    : "=a" (__dummy_res) \
    : "0" (__NR_##name),"D" ((long)(arg1)) : __syscall_clobber ); \
}



/*
 * Normal macros
 */ 
#define __syscall_return(type, res) \
do { \
    if ((unsigned long)(res) >= (unsigned long)(-127)) { \
        errno = -(res); \
        res = -1; \
    } \
    return (type) (res); \
} while (0)


#define _syscall0(type,name) \
type name(void) \
{ \
long __res; \
__asm__ volatile (__syscall \
    : "=a" (__res) \
    : "0" (__NR_##name) : __syscall_clobber ); \
__syscall_return(type,__res); \
}

#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
long __res; \
__asm__ volatile (__syscall \
    : "=a" (__res) \
    : "0" (__NR_##name),"D" ((long)(arg1)) : __syscall_clobber ); \
__syscall_return(type,__res); \
}

#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
long __res; \
__asm__ volatile (__syscall \
    : "=a" (__res) \
    : "0" (__NR_##name),"D" ((long)(arg1)),"S" ((long)(arg2)) : __syscall_clobber ); \
__syscall_return(type,__res); \
}

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1,type2 arg2,type3 arg3) \
{ \
long __res; \
__asm__ volatile (__syscall \
    : "=a" (__res) \
    : "0" (__NR_##name),"D" ((long)(arg1)),"S" ((long)(arg2)), \
          "d" ((long)(arg3)) : __syscall_clobber); \
__syscall_return(type,__res); \
}

#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
type name (type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
long __res; \
__asm__ volatile ("movq %5,%%r10 ;" __syscall \
    : "=a" (__res) \
    : "0" (__NR_##name),"D" ((long)(arg1)),"S" ((long)(arg2)), \
      "d" ((long)(arg3)),"g" ((long)(arg4)) : __syscall_clobber,"r10" ); \
__syscall_return(type,__res); \
} 

#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
      type5,arg5) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5) \
{ \
long __res; \
__asm__ volatile ("movq %5,%%r10 ; movq %6,%%r8 ; " __syscall \
    : "=a" (__res) \
    : "0" (__NR_##name),"D" ((long)(arg1)),"S" ((long)(arg2)), \
      "d" ((long)(arg3)),"g" ((long)(arg4)),"g" ((long)(arg5)) : \
    __syscall_clobber,"r8","r10" ); \
__syscall_return(type,__res); \
}

#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
      type5,arg5,type6,arg6) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5,type6 arg6) \
{ \
long __res; \
__asm__ volatile ("movq %5,%%r10 ; movq %6,%%r8 ; movq %7,%%r9 ; " __syscall \
    : "=a" (__res) \
    : "0" (__NR_##name),"D" ((long)(arg1)),"S" ((long)(arg2)), \
      "d" ((long)(arg3)), "g" ((long)(arg4)), "g" ((long)(arg5)), \
      "g" ((long)(arg6)) : \
    __syscall_clobber,"r8","r10","r9" ); \
__syscall_return(type,__res); \
}


#else 
    #error "No syscall macros found"
#endif

#endif

