#ifndef __BTIMER_RDTSC_H__
#define __BTIMER_RDTSC_H__

#ifndef CPU_BITS
#define CPU_BITS 32
#endif

#if (CPU_BITS == 32)
#include "btimer_rdtsc32.h"
#endif

#if (CPU_BITS == 64)
#include "btimer_rdtsc64.h"
#endif


#endif /* __BTIMER_RDTSC_H__ */
