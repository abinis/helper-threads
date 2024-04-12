#ifndef STATS_H__
#define STATS_H__

#define NUM_DATA_POINTS 1000

typedef struct {
    int nthreads;
    unsigned int *cycles_so_far;
} stats_per_thr_t;

#endif
