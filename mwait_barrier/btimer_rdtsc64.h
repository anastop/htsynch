#ifndef BTIMER64_H
#define BTIMER64_H

/*
 * btimer64.h
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
   unsigned long _hi; \
 \
   __asm__ __volatile__ ("rdtsc" \
                         : "=a"((x)), "=d"(_hi) \
   ); \
   (x) |= (_hi << 32); \
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
            _timer->total = 0; \
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
            _timer->total += _timer->stop - _timer->start; \
            _timer->loops++; \
}

#define rdtsc_shift(x, shift) ((x) >> (shift))


#define TIMER_CLEAR(timer) { \
            struct timer_s *_timer = (timer); \
            _timer->loops = 0; \
            _timer->total = 0; \
}

#define TIMER_TOTAL(timer) ({ \
            struct timer_s *_timer = (timer); \
            (double) _timer->total; \
})

#define TIMER_AVE(timer) ({ \
            struct timer_s *_timer = (timer); \
            double _ave; \
            _ave = (double) _timer->total; \
            _ave /= (double) _timer->loops; \
})

#define TIMER_REPORT(timer, message) { \
            struct timer_s *_timer = (timer); \
            printf("%s:%s: 0x%016lX,  %lu\n", #timer, message, \
                   _timer->total, _timer->total \
                  ); \
}

#define TIMER_STARTED(timer) ({ \
           struct timer_s *_timer = (timer); \
           unsigned long _started; \
           _started = _timer->start; \
}) 

#define TIMER_STOPPED(timer) ({ \
           struct timer_s *_timer = (timer); \
           unsigned long _stopped; \
           _stopped = _timer->stop; \
})


typedef struct timer_s {
       unsigned long start;
       unsigned long stop;
       unsigned long total;
       unsigned long loops;
} mytimer_t;


#endif
