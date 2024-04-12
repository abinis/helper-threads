#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "edge_mt_stats.h"

void * calloc_safe(size_t size)
{
	void *p;

	if ( !(p = malloc(size)) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

        memset(p, 0, size);

	return p;
}

mt_stats_t * mt_stats_init(int iterations, int maxthreads, int numdatapoints)
{
    mt_stats_t *statsp;
    int i, t, d;

    statsp = malloc(sizeof(mt_stats_t));
    if ( !statsp ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
		exit(EXIT_FAILURE);
    }

    statsp->iterations = iterations;
    statsp->maxthreads = maxthreads;
    statsp->numdatapoints = numdatapoints;

    //rec->stats = calloc(iterations*numdatapoints*maxthreads, sizeof(mt_stats_t));
    statsp->stats = malloc(iterations*sizeof(mt_stats_record_t**));
    if ( !(statsp->stats) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    for ( i = 0; i < iterations; i++ ) {
        statsp->stats[i] = malloc(maxthreads*sizeof(mt_stats_record_t*));
        if ( !(statsp->stats[i]) ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        for ( t = 0; t < maxthreads; t++ ) {
            statsp->stats[i][t] = calloc(numdatapoints,sizeof(mt_stats_record_t));
            if ( !(statsp->stats[i][t]) ) {
                fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
                exit(EXIT_FAILURE);
            }
            for ( d = 0; d < numdatapoints; d++ ) {
                // hopefully we don't need to manually set the various counters,
                // e.g. seconds, cycles_main, cycles_helper etc, to 0, thanks to
                // calloc (instead of malloc) :)
                //statsp->stats[i][t][d].nthreads = t+1;
                statsp->stats[i][t][d].cycles_per_thr = calloc(t+1,sizeof(unsigned int));
            }
        }
    }

    //rec->iterations = iterations;
    //rec->numdatapoints = numdatapoints;
    //rec->maxthreads = maxthreads;

    return statsp;
}

void mt_stats_record_fill(mt_stats_t *statsp, 
                          int iteration, int nthreads, int datapoint,
                          unsigned int until, 
                          double seconds,
                          unsigned int msf_edges,
                          unsigned int cycles_main,
                          unsigned int cycles_helper,
                          unsigned int cycles_skipped)
{
    assert(statsp);

    mt_stats_record_t *where = &(statsp->stats[iteration][nthreads][datapoint]);
    where->until = until;
    where->seconds = seconds;
    where->msf_edges = msf_edges;
    where->cycles_main = cycles_main;
    where->cycles_helper = cycles_helper;
    where->cycles_skipped = cycles_skipped;
}

void mt_stats_partial_progress(mt_stats_t *statsp, 
                               int iteration, int nthreads, 
                               unsigned int nedges,
                               unsigned int msf_edges_total)
{
    assert(statsp);

    int numdatapoints = statsp->numdatapoints;
    int helper_threads = nthreads - 1;
    int ht;
    // which record do we look into? 
    mt_stats_record_t *rec = statsp->stats[iteration][nthreads];
    unsigned int cycle_edges_total = nedges - msf_edges_total;

    //double *msf_edge_percentage;
    double cycle_edge_percentage_main;
    double cycle_edge_percentage_helper;
    //double **cycle_edge_percentage_per_ht;
    double msf_edge_percentage_t;
    double cycle_edge_percentage_t;


    //msf_edge_percentage = calloc_safe( numdatapoints * sizeof(double) );
    //cycle_edge_percentage_main = calloc_safe( numdatapoints * sizeof(double) );
    //cycle_edge_percentage_helper = calloc_safe( numdatapoints * sizeof(double) );
    /*
    cycle_edge_percentage_per_ht = malloc( numdatapoints * sizeof(double*));
    if ( !cycle_edge_percentage_per_ht ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    */

    //msf_edge_percentage_t = calloc_safe( numdatapoints * sizeof(double) );
    //cycle_edge_percentage_t = calloc_safe( numdatapoints * sizeof(double) );

    unsigned int scale = 60U;
    unsigned int d;
    char bar[61];
    int i;
    for ( i = 0; i <= 60; i++ )
        bar[i] = ' ';
    //printf("after bar initial 'zero'-ing :P\n");

    for ( d = 0; d < numdatapoints; d++ ) {

        msf_edge_percentage_t = (double)(rec[d].msf_edges) / (double)msf_edges_total;
        int msf_mark = (int)round(msf_edge_percentage_t*scale);
        bar[msf_mark] = 'x';

        cycle_edge_percentage_t = (double)(rec[d].cycles_main + rec[d].cycles_helper)
                                     / (double)cycle_edges_total;
        int cycle_tot_mark = (int)round(cycle_edge_percentage_t*scale);
        bar[cycle_tot_mark] = 'o';

        cycle_edge_percentage_main = (double)(rec[d].cycles_main) / (double)cycle_edges_total;
        int cycle_main_mark = (int)round(cycle_edge_percentage_main*scale);
        bar[cycle_main_mark] = 'm';

        cycle_edge_percentage_helper = (double)(rec[d].cycles_helper) / (double)cycle_edges_total;
        int cycle_help_mark = (int)round(cycle_edge_percentage_helper*scale);
        bar[cycle_help_mark] = 'n';

        /*
        cycle_edge_percentage_per_ht[d] = calloc_safe( helper_threads * sizeof(double) );
        for ( ht = 0; ht < helper_threads; ht++ ) {
            cycle_edge_percentage_per_ht[d][ht] = (double)(rec[d].cycles_per_thr[ht+1]) / (double)cycle_edges_total;
        }
        */

        printf("%6.2lf |", ((double)(d+1) / (double)numdatapoints)*100);
        for ( i = 0; i <= 60; i++ )
            printf("%c", bar[i]);
        printf("\n");

        // Prepare for next datapoint :)
        for ( i = 0; i <= 60; i++ )
            bar[i] = ' ';
    }

}

/*
void mt_stats_partial_progress(unsigned int nedges, char *edge_color_main,
                               unsigned int numthreads,
                               unsigned int *until,
                               unsigned int total_msf_edges)
{
    unsigned int e;
    double *msf_edge_percentage;
    double *cycle_edge_percentage;
    double *msf_edge_percentage_t;
    double *cycle_edge_percentage_t;

    msf_edge_percentage = calloc_safe( 
}
*/

void mt_stats_destroy(mt_stats_t *statsp)
{
    assert(statsp);

    int i, t, d;
    for ( i = 0; i < statsp->iterations; i++ ) {
        for ( t = 0; t < statsp->maxthreads; t++ ) {
            for ( d = 0; d < statsp->numdatapoints; d++ )
                free(statsp->stats[i][t][d].cycles_per_thr);
            free(statsp->stats[i][t]);
        }
        free(statsp->stats[i]);
    }
    free(statsp->stats);
    free(statsp);
}
