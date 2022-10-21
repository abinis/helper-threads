/**
 * @file
 * Time-measuring inline functions using TSC
 */ 

#ifndef TSC_X86_64_H_
#define TSC_X86_64_H_

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    uint64_t tic;
    uint64_t toc;
    uint64_t total;
    uint64_t invocs;
} tsctimer_t;

static inline double timer_read_hz() {
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

static inline void timer_clear(tsctimer_t *t)
{
    t->invocs = 0;
    t->total = 0;
}

static inline void timer_start(tsctimer_t *t)
{
    uint64_t hi;

    // rdtsc returns the tsc in edx:eax
    __asm__ __volatile__ ( "rdtsc"
                           : "=a"(t->tic), "=d"(hi) 
                         );
    t->tic |= (hi << 32);
}

static inline void timer_stop(tsctimer_t *t)
{
    uint64_t hi;

    __asm__ __volatile__ ( "rdtsc"
                           : "=a"(t->toc), "=d"(hi) 
                         );
    t->toc |= (hi << 32);
    t->total += t->toc - t->tic;
    t->invocs++;
}

static inline uint64_t timer_read()
{
    uint64_t hi, lo;
    __asm__ __volatile__ ( "rdtsc"
                           : "=a"(lo), "=d"(hi)
                         );
    return ( (hi << 32) | lo );
}

static inline double timer_total(tsctimer_t *t)
{
    return (double)t->total;
}

static inline double timer_average(tsctimer_t *t)
{
    return (double)t->total / (double)t->invocs;
}

/**
 * Spin for a specific number of cycles
 * @param ncycles number of cycles
 */ 
static inline void spin_for_cycles(unsigned long ncycles)
{
    register uint64_t end = timer_read() + (uint64_t)ncycles;
    while ( timer_read() < end ) ;
}

#endif
