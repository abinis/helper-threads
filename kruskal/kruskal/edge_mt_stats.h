#ifndef EDGE_MT_STATS_H__
#define EDGE_MT_STATS_H__

typedef struct {
    //int nthreads; /* probably not needed */
    unsigned int until;
    double seconds;
    unsigned int msf_edges;
    unsigned int cycles_main;
    unsigned int cycles_helper;
    unsigned int cycles_skipped;
    unsigned int *cycles_per_thr;
} mt_stats_record_t;

typedef struct {
    int iterations;
    int maxthreads;
    int numdatapoints;
    mt_stats_record_t ***stats;
} mt_stats_t;

mt_stats_t * mt_stats_init(int iterations, int maxthreads, int numdatapoints);

void mt_stats_record_fill(mt_stats_t *statsp, 
                          int iteration, int nthreads, int datapoint,
                          unsigned int until, 
                          double seconds,
                          unsigned int msf_edges,
                          unsigned int cycles_main,
                          unsigned int cycles_helper,
                          unsigned int cycles_skipped);

void mt_stats_partial_progress(mt_stats_t *statsp, 
                               int iteration, int nthreads, 
                               unsigned int nedges,
                               unsigned int msf_edges_total);

/*
void mt_stats_partial_progress(unsigned int nedges, char *edge_color_main,
                               unsigned int numthreads,
                               unsigned int *until,
                               unsigned int total_msf_edges);
*/

void mt_stats_destroy(mt_stats_t *statsp);

#endif
