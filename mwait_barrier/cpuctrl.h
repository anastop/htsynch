#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>


/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define CPUCTRL_IOC_MAGIC  'k'
/* Please use a different 8-bit number in your code */

#define CPUCTRL_IOC_HALT                _IO(CPUCTRL_IOC_MAGIC, 0)
#define CPUCTRL_IOC_WAKEUP              _IO(CPUCTRL_IOC_MAGIC, 1)
#define CPUCTRL_IOC_DUMMY               _IO(CPUCTRL_IOC_MAGIC, 2)
#define CPUCTRL_IOC_SET_CPU_TO_WAKEUP   _IOW(CPUCTRL_IOC_MAGIC, 3, int)
#define CPUCTRL_IOC_WAKEUP_ALLBUTSELF   _IO(CPUCTRL_IOC_MAGIC, 4)
#define CPUCTRL_IOC_GET_SMPID           _IO(CPUCTRL_IOC_MAGIC, 5)