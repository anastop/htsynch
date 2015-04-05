/*
 * btimer_rdtsc.h
 *
 * gtsouk@cslab.ece.ntua.gr
 *
 */


#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <wait.h>
#include <string.h>


#define rdtsc(x) { \
   unsigned long *_t = (x); \
 \
   __asm__ __volatile__ ("rdtsc" \
                         : "=a"(_t[0]), "=d"(_t[1]) \
   ); \
 \
}

static inline double read_hz() {
    char *buf, *ptr;
    int fd;
    double hz;

    buf = malloc(4096);
    if (!buf) exit(1);

    fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) exit(1);

    read(fd, buf, 4096);
    ptr = strstr(buf, "cpu MHz");
    while (*ptr++ != ':');

    hz = strtod(ptr, NULL);
    hz *= 1000000;

    free(buf);

    return hz;
}

static inline int read_nr_cpus() {
    return WEXITSTATUS(system("exit $(cat /proc/cpuinfo | grep processor | wc -l)"));
}

static inline int read_nr_siblings() {
    return WEXITSTATUS(system("exit $(cat /proc/cpuinfo | grep siblings | head -1 | cut -d: -f 2)"));
}


#define TIMER_CLEAR(timer) { \
    struct timer_s *_timer = (timer); \
    _timer->loops = 0; \
    _timer->total[0] = 0; \
    _timer->total[1] = 0; \
}

#define TIMER_ON TIMER_START
#define TIMER_START(timer) { \
    struct timer_s *_timer = (timer); \
    rdtsc(_timer->start); \
}

#define TIMER_OFF TIMER_STOP
#define TIMER_STOP(timer) { \
    struct timer_s *_timer = (timer); \
    rdtsc(_timer->stop); \
    __asm__ ("subl %2, %4\n" \
             "sbbl %3, %5\n" \
             "addl %4, %0\n" \
             "adcl %5, %1\n" \
             : "=r"(_timer->total[0]), "=r"(_timer->total[1]) \
             : "r"(_timer->start[0]), "r"(_timer->start[1]), "r"(_timer->stop[0]), "r"(_timer->stop[1]), "0"(_timer->total[0]), "1"(_timer->total[1])  \
            ); \
            _timer->loops++; \
}

#define rdtsc_shift(x, shift) ({ \
       unsigned long _hi = (x)[1], _lo = (x)[0], _bits = (shift); \
             lo >>= bits; \
             hi <<= (32 - bits); \
             lo |= hi; \
             (x)[0] = lo; (x)[1] = hi; \
})


#define TIMER_TOTAL(timer) ({ \
            struct timer_s *_timer = (timer); \
            double _total; \
            _total = (double) _timer->total[1]*4.294967296e9; \
            _total += (double) _timer->total[0]; \
})

#define TIMER_AVE(timer) ({ \
            struct timer_s *_timer = (timer); \
            double _ave; \
            _ave = (double) _timer->total[0]; \
            _ave += (double) _timer->total[1]*4.294967296e9; \
            _ave /= (double) _timer->loops; \
})

#define TIMER_REPORT(timer, message) { \
            struct timer_s *_timer = (timer); \
            printf("%s:%s: 0x%08lX%08lX, %f\n", #timer, message, \
                   _timer->total[1], _timer->total[0], \
                   TIMER_TOTAL(timer) \
                   ); \
}

#define TIMER_STARTED(timer) ({ \
           struct timer_s *_timer = (timer); \
           unsigned long _started; \
           _started = _timer->start[0]; \
})

#define TIMER_STOPPED(timer) ({ \
           struct timer_s *_timer = (timer); \
           unsigned long _stopped; \
           _stopped = _timer->stop[0]; \
})


typedef struct timer_s {
       unsigned long start[2];
       unsigned long stop[2];
       unsigned long total[2];
       unsigned long loops;
} mytimer_t;
