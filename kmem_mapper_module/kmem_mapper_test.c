#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define NPAGES 1

/* this is a test program that opens the kmem_mapper_drv.
   It reads out values of the kmalloc() and vmalloc()
   allocated areas and checks for correctness.
   You need a device special file to access the driver.
   The device special file is called 'node' and searched
   in the current directory.
   To create it
   - load the driver
     'insmod kmem_mapper_mod.o'
   - find the major number assigned to the driver
     'grep mmapdrv /proc/devices'
   - and create the special file (assuming major number 254)
     'mknod /dev/kmem_mapper c 201 0'
*/

int main(void)
{
    int fd, i;
    char *kadr;

    int len = NPAGES * getpagesize();

    if ((fd = open("/dev/kmem_mapper", O_RDWR | O_SYNC)) < 0) {
        perror("open");
        exit(-1);
    }

    kadr = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, fd, len);
    if (kadr == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    fprintf(stderr, "Mmap address: 0x%lx\n", (unsigned long)kadr);
    
    fprintf(stderr, "Mapped memory area contents (1024 first bytes).\n");   
    fprintf(stderr, "Reading memory: ");    
    for (i = 0; i < 1024; i++) {
        if(!(i%100))
            fprintf(stderr, "\n");
        fprintf(stderr, "%c ", kadr[i]); 
    }
    fprintf(stderr,"\n");

    fprintf(stderr, "Changing mapped memory area contents (10 first bytes) to ");   
    for (i = 0; i < 10; i++) {
        kadr[i] = (char)(i+110);
        fprintf(stderr, "%c", kadr[i]); 
    }
    fprintf(stderr,"\n");

    close(fd);
    return (0);
}
