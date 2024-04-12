/**
 * @file
 * Various utility functions
 */ 
#define _GNU_SOURCE

#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Trims each occurence of a char in a string
 * @param src source string
 * @param tc char to trim
 */ 
void trim(char *src, const char tc)
{
    unsigned int i, j;

    i = j = 0;

    while ( src[i] ) {
        if ( src[i] != tc )  
            src[j++] = src[i++];
        else 
            i++;
    }

    src[j] = '\0';
}


/**
 * Allocate memory at a given alignment boundary, or die
 * @param size number of bytes to allocate
 * @param alignment alignment boundary (must be power of 2)
 * @return pointer to allocated memory
 */
void* malloc_aligned(size_t size, size_t alignment)
{
	void *p;

	if ( !(p = malloc(size + alignment-1)) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

	// Perform alignment 
	p = (void *) (((unsigned long)p + alignment - 1) & ~(alignment - 1));
										
	return p;
}

/**
 * Malloc or die 
 * @param size number of bytes to allocate
 * @return pointer to allocated memory
 */
void* malloc_safe(size_t size)
{
	void *p;

	if ( !(p = malloc(size)) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

	return p;
}


#define BUFFERED_READ_SIZE 8192
/**
 * Reads up to READ_BUT_SIZE characters from a file and 
 * returns them one-by-one at every next invocation
 * @param fd file descriptor
 * @param ptr pointer to buffer where chars are stored (one at a time)
 * @return 1 if a char was read fine, 0 if EOF encountered, -1 if error
 */
static inline ssize_t buffered_read(int fd, unsigned char *ptr)
{
	static int read_cnt = 0;
	static unsigned char *read_ptr;
	static unsigned char read_buf[BUFFERED_READ_SIZE];

    // Every time we actually read from the file, read_cnt gets a 
    // value up to BUFFERED_READ_SIZE, so the branch is not taken in 
    // next invocations
    if ( read_cnt <= 0 ) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0 ) {
            if ( errno == EINTR )
                goto again;
            return -1;
		} else if ( read_cnt == 0 ) // EOF       
            return 0;
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return 1;
}


/**
 * Reads a '\n'-terminated line from a file into a target buffer.
 * Uses buffered read for better performance.
 * @param fd file descriptor
 * @param vptr pointer to the target buffer 
 * @param maxlen the maximum length that the line can have (actually, 
 *        it is maxlen-1 taking into account the empy character)
 * @return number of bytes read (up to maxline)
 */
ssize_t readline(int fd, char *vptr, size_t maxlen)
{
	ssize_t n, rc;
	unsigned char c, *ptr;
	
	ptr = (unsigned char *)vptr;
	for ( n = 1; n < maxlen; n++ ) {
        if ( (rc = buffered_read(fd, &c) ) == 1 ) {
            *ptr++ = c;
            // if at end of line, stop reading
            //printf("read #%ld: %c\n", n, c);
            if (c == '\n')
                break;
        } else if ( rc == 0 ) {
            // EOF, without having read anything
            if ( n == 1 )
                return 0;
            // EOF, having read something
            else
				break;                  
        } else
            return -1; // error      
	}

        // commented-out the condition below,
        // always null-terminate the target buffer!
        // *actually: 1. don't strip the trailing newline
        //            2. append a '\0' at the end
	//if ( *(ptr-1) == '\n' )             
        //*(ptr-1) = '\0';
            *(ptr) = '\0';

        // if no end of line was reached, i.e. the
        // line is longer than GRAPH_MAX_LINE_LENGTH,
        // continue reading and discarding the remainder
        // of the line
        // note: alternatively, the caller could be
        // responsible for sscanf-ing the returned buffer
        // to see if they found what they wanted in it,
        // and if not, call readline repeatedly :P
        //if ( n == maxlen && *(ptr-1) != '\n' ) {
        //    do {
        //        rc = buffered_read(fd, &c);
        //        printf("read: %c\n", c);
        //        if ( rc < 0 )
        //            return -1; // error
        //    } while ( rc == 1 && c != '\n' );
        //}

	return n;
}


/**
 * Generates a maximal period pseudo-random sequence of integers
 * http://en.wikipedia.org/wiki/Linear_feedback_shift_register
 */ 
inline unsigned int galois_lfsr()
{
    static unsigned int lfsr = 1;
        
    // taps: 32 31 29 1 
    // characteristic polynomial: x^32 + x^31 + x^29 + x + 1 
    lfsr = (lfsr >> 1) ^ (unsigned int)((0 - (lfsr & 1u)) & 0xd0000001u); 

    return lfsr;
}

/**
 * Marsaglia xorshift PRNG with period 2^128-1
 * @note: http://en.wikipedia.org/wiki/Xorshift
 * @return pseudo-random unsigned integer
 */ 
inline unsigned int marsaglia_prng(void)
{
    static unsigned int x = 123456789,
                        y = 362436069,
                        z = 521288629,
                        w = 88675123;
    unsigned int t;
    t = x ^ (x << 11);
    x = y;
    y = z;
    z = w;
    return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

/**
 * Swaps two data objects of the same type
 * @param x pointer to 1st object
 * @param y pointer to 2nd object
 * @param elem_size number of bytes of object
 */ 
void swap(void *x, void *y, size_t elem_size)
{
    char *_x, *_y, *tmp;

    _x = (char*)x;
    _y = (char*)y;

    tmp = malloc(elem_size);
    if ( !tmp ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    
    memcpy(tmp, _x, elem_size);
    memcpy(_x, _y, elem_size);
    memcpy(_y, tmp, elem_size);

    printf("swap: sizeof edge_t = %lu\n", elem_size);
    

    free(tmp);

    printf("swap: after free\n");
}



static char* _dummy_buf;
static unsigned long _flush_bytes;
static pthread_barrier_t _bar;
static void* _flush_fun(void *args)
{
    unsigned long i;

    pthread_barrier_wait(&_bar);
    for ( i = 0; i < _flush_bytes; i+=64 ) {
        // Fetch byte and modify it.
        // Eventually, this will fill all TLB entries with translations 
        // relevant to _dummy_buf, evicting any other translations 
        // we would like to flush.
        _dummy_buf[i]++;

        // Invalidate corresponding cache line from all levels.
        __asm__ __volatile__ ("clflush %0; mfence\n"
                              : 
                              : "m" (_dummy_buf[i])
                              : "memory");
    }
    pthread_barrier_wait(&_bar);
    pthread_exit(NULL);
}

/**
 * Flushes all caches and TLBs on all processors. 
 * @param num_proc number of processors in the system
 * @param flush_bytes cache size in bytes to flush 
 */
void flush_caches(int num_proc, unsigned long flush_bytes)
{
    _flush_bytes = flush_bytes;
    pthread_t *tids;
    pthread_attr_t *attr;
    cpu_set_t *cpusets;
    int i;

    _dummy_buf = (char*)malloc_safe(flush_bytes);
    tids = (pthread_t*)malloc_safe(num_proc * sizeof(pthread_t));
    attr = (pthread_attr_t*)malloc_safe(num_proc * sizeof(pthread_attr_t));
    cpusets = (cpu_set_t*)malloc_safe(num_proc * sizeof(cpu_set_t));

    for ( i = 0; i < num_proc; i++ ) {
        CPU_ZERO(&cpusets[i]);
        CPU_SET(i, &cpusets[i]);
    }

    pthread_barrier_init(&_bar, NULL, num_proc);

    for ( i = 0; i < num_proc; i++ ) {
        pthread_attr_init(&attr[i]);
        pthread_attr_setaffinity_np(&attr[i], 
                                    sizeof(cpusets[i]), 
                                    &cpusets[i]);
        pthread_create(&tids[i], &attr[i], _flush_fun, (void*)0);
    }
    for ( i = 0; i < num_proc; i++ ) {
        pthread_join(tids[i], NULL);
        pthread_attr_destroy(&attr[i]);
    }
    pthread_barrier_destroy(&_bar);
    free(_dummy_buf);
    free(cpusets);
    free(attr);
    free(tids); 
}

