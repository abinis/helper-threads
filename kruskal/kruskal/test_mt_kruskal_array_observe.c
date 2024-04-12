/**
 * @file
 * Test Kruskal multithreaded implementations
 */ 

#define _GNU_SOURCE

/**
 * Define macros to choose between multiple
 * (well, two for the time being :P) processor_map
 * implementations/header files
 */
//#include PROCMAP_H

#include <time.h>

#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "graph/graph.h"
#include "graph/adjlist.h"
#include "kruskal_array.h"
#include "machine/tsc_x86_64.h"
#include "mt_kruskal.h"
#include "processor_map/processor_map.h"
//#define PROCMAP_H "processor_map/processor_map.h"
//#include PROCMAP_H
#include "util/util.h"

//how many cycles the main thread skipped thanks to helper thread work
unsigned int cycles_skipped;
//how many cycles the main thread found
volatile unsigned int cycles_main;

//running count of cycles found per thread
volatile unsigned int *cycles_so_far;
unsigned int ***thread_stats;
unsigned int interval;
int nthreads;
int iter;
struct timespec time_interval;

pthread_barrier_t bar;
tsctimer_t tim;

adjlist_stats_t stats;
edgelist_t *el;
adjlist_t *al;
union_find_node_t *array;
unsigned int *edge_membership; 
char *edge_color_main;
char *edge_color_helper;

void touch_structures(edgelist_t *_el,
                             adjlist_t *_al,
                             union_find_node_t *_farray,
                             char *_edge_color_main,
                             char *_edge_color_helper)
{
    unsigned int e, v;

    for ( e = 0; e < _el->nedges; e++ ) {
        _edge_color_main[e] += 0;
        _edge_color_helper[e] += 0;
        _el->edge_array[e].weight += 0.0;
    }

    for ( v = 0; v < al->nvertices; v++ ) 
        _farray[v].rank += 0;
}
                             

int main(int argc, char **argv)
{
    targs_t *targs;
    pthread_t *tids;
    pthread_attr_t *attr;
    procmap_t *pi;
    unsigned long llc_level, llc_bytes;
    int p, c, t, i, j, k, e, /*iter,*/ maxthreads, 
        is_undirected, mapping,
        iterations = 1,
        msf_edge_count = 0;
    char graphfile[256];

    if ( argc < 4 ) {
        fprintf(stderr, "Usage: %s <graphfile> <maxthreads> "
                        "<mapping= 0:cpt, 1:pct> [iterations(default=1)]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    sprintf(graphfile, "%s", argv[1]);
    maxthreads = atoi(argv[2]);
    mapping = atoi(argv[3]);

    if ( argc > 4 ) 
        iterations = atoi(argv[4]);

    //printf("Entering procmap_init...\n");
    // Find processor topology and configure thread affinity
    pi = procmap_init();
    assert(pi);
    //printf("...procmap_init ok!\n");
    //printf("On second thought, let's check (report)...\n");
    //procmap_report(pi);
    //printf("...report ok!\n");

    cpu_set_t cpusets[pi->num_cpus];
    //printf("pi->num_cpus = %d\n", pi->num_cpus);
    

    // Get LLC bytes
    llc_level = pi->flat_threads[0].num_caches - 1;
    //printf("llc_level read = %lu\n", llc_level);
    llc_bytes = pi->flat_threads[0].cache[llc_level].size;
    //printf("llc_bytes read = %lu\n", llc_bytes);
    fprintf(stdout, "LLC bytes: %lu\n", llc_bytes);
    llc_bytes = llc_bytes * 10;  // bytes to flush

  
    // Configure thread mapping
    i = 0;
    if ( mapping == 0 ) {
        fprintf(stdout, "Thread fill order: cores>packages>threads\n");
        for ( t = 0; t < pi->num_threads_per_core; t++ ) {
            for ( p = pi->num_packages-1; p >= 0; p-- ) {
                for ( c = 0; c < pi->num_cores_per_package; c++ ) {
                    //printf("inside inner cpt loop\n");
                    int cpu_id = pi->package[p].core[c].thread[t]->cpu_id;
                    //printf("pi access successful\n");
                    CPU_ZERO(&cpusets[i]);
                    CPU_SET(cpu_id, &cpusets[i]);
                    fprintf(stdout, "Thread %d @ package %d, core %d, "
                                    "hw thread %d (cpuid: %d)\n",
                                    i, p, c, t, cpu_id);
                    i++;
                }
            }    
        }
    } else if ( mapping == 1 ) {
        fprintf(stdout, "Thread fill order: packages>cores>threads\n");
        for ( t = 0; t < pi->num_threads_per_core; t++ ) {
            for ( c = 0; c < pi->num_cores_per_package; c++ ) {
                for ( p = pi->num_packages-1; p >= 0; p-- ) {
                    //printf("inside inner pct loop\n");
                    int cpu_id = pi->package[p].core[c].thread[t]->cpu_id;
                    CPU_ZERO(&cpusets[i]);
                    CPU_SET(cpu_id, &cpusets[i]);
                    fprintf(stdout, "Thread %d @ package %d, core %d, "
                                    "hw thread %d (cpuid: %d)\n",
                                    i, p, c, t, cpu_id);
                    i++;
                }
            }    
        }
    } else {
        fprintf(stderr, "Unknown mapping configuration");
        exit(EXIT_FAILURE);
    }
    fprintf(stdout, "\n");

    // Note: on NUMA machines, all malloc calls that follow
    // a _setaffinity system call reserve pages on the memory node 
    // which is local to the affine processor. For this reason, we 
    // should bind the main context (i.e. the context where all 
    // allocations occur) to the same processor as the main thread 
    // will run on later, before any allocations.
    sched_setaffinity(getpid(), sizeof(cpusets[0]), &cpusets[0]);

    //printf("phew! made it to the graph init!\n");

    timer_clear(&tim);
    timer_start(&tim);
    // Init adjacency list
    adjlist_init_stats(&stats);
    is_undirected = 1;
    al = adjlist_read(graphfile, &stats, is_undirected);
    timer_stop(&tim);
    fprintf(stdout, "Read graph\n\n");
    double hz = timer_read_hz();
    fprintf(stdout, "adjlist_read         cycles:%lf  freq:%.0lf seconds:%lf\n", 
                    timer_total(&tim), hz, timer_total(&tim)/hz);

    timer_clear(&tim);
    timer_start(&tim);
    // Create edge list from adjacency list
    el = edgelist_create(al);
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_create      cycles:%lf  freq:%.0lf seconds:%lf\n", 
                    timer_total(&tim), hz, timer_total(&tim)/hz);
 
    timer_clear(&tim);
    timer_start(&tim);
    // Sort edge list
    kruskal_sort_edges(el); 
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal_sort_edges   cycles:%lf  freq:%.0lf seconds:%lf\n", 
                    timer_total(&tim), hz, timer_total(&tim)/hz);

    fprintf(stdout, "\n");

    // Allocate structure to hold stats for all thread counts, for all
    // iterations :D
    thread_stats = malloc(iterations*sizeof(unsigned int**));
    if ( !thread_stats ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    int numrows = maxthreads*(maxthreads+1)/2;
    for ( i = 0; i < iterations; i++ ) {
        thread_stats[i] = malloc(numrows*sizeof(unsigned int*));
        if ( !thread_stats[i] ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        for ( j = 0; j < numrows; j++ ) {
            thread_stats[i][j] = calloc(NUM_DATA_POINTS,sizeof(unsigned int));
            if ( !thread_stats[i][j] ) {
                fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
                exit(EXIT_FAILURE);
            }
        }
    }
    // Observer thread takes a snapshot of the progress every that many nanos :)
    // interval = el->nedges / NUM_DATA_POINTS;
    time_interval = { .tv_sec = 0, .tv_nsec = 1000000 };
    // Stores running number of cycles found by each thread ;)
    cycles_so_far = calloc(maxthreads,sizeof(unsigned int));
    if ( !cycles_so_far ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    //printf("about to start iterations\n");
    // Run as many iterations as specified, default is one
    for ( iter = 0; iter < iterations; iter++ ) {
        // Run MT algorithm for different thread numbers
        for ( nthreads = 1; nthreads <= maxthreads; nthreads++ ) {
            fprintf(stdout, "nthreads:%d  graph:%s  mapping:%s  ", 
                            nthreads, graphfile, mapping == 0 ? "cpt":"pct");
  
            // Perform initializations 
            kruskal_init(el, al, &array, &edge_membership);
            kruskal_helper_init(el, &edge_color_main, &edge_color_helper);
            
            flush_caches(pi->num_cpus, llc_bytes);

            // Allocate thread structures 
            tids = (pthread_t*)malloc_safe( (nthreads+1) * sizeof(pthread_t) );
            targs = (targs_t*)malloc_safe( (nthreads+1) * sizeof(targs_t)); 
            attr = (pthread_attr_t*)malloc_safe( (nthreads+1) * sizeof(pthread_attr_t)); 
            // nthreads *+1* to account for the observer thread
            pthread_barrier_init(&bar, NULL, nthreads+1);

            timer_clear(&tim);

            // Create threads
            assert(el->nedges > nthreads);
            int chunk_size = el->nedges / nthreads;
            
            for ( i = 0; i < nthreads; i++ ) {
                if ( i < 1 ) {
                    targs[i].type = MAIN_THR;
                    targs[i].begin = 0;
                    targs[i].end = el->nedges;
                } else {
                    targs[i].type = HELPER_THR;
                    targs[i].end = i * chunk_size;
                    targs[i].begin = targs[i].end + chunk_size; 
                }
                targs[i].id = i;
                // extra args for keeping stats
                targs[i].ht_loop_count = 0;
                memset(targs[i].ht_cycles_per_loop, 0, TARGS_LOOP_ARR_SZ);
                targs[i].why = 0;

                pthread_attr_init(&attr[i]);
                pthread_attr_setaffinity_np(&attr[i], 
                                            sizeof(cpusets[i]), 
                                            &cpusets[i]);
                pthread_create(&tids[i], &attr[i], kruskal_ht, (void*)&targs[i]);
            }

            // Fill in observer pthread things
            targs[nthreads].type = OBSERVER_THR;
            pthread_attr_init(&attr[nthreads]);
            pthread_attr_setaffinity_np(&attr[nthreads], 
                                        sizeof(cpusets[nthreads]), 
                                        &cpusets[nthreads]);
            pthread_create(&tids[nthreads], &attr[nthreads], kruskal_ht, (void*)&targs[nthreads]);

            // total cycles found by all helper threads
            unsigned int ht_total_cycles = 0;
            for ( i = 0; i < nthreads; i++ ) {
                // TODO: should be out of the critical (timed) region ->
                // move stats to independent structure, outside of targs
                // Also TODO, this always prints 0 main_fin for tid 0 (main) :P
                // will be fixed with the single struct where everybody reports
                kruskal_helper_print_stats((void*)&targs[i]);
                ht_total_cycles += kruskal_helper_get_total_cycles((void*)&targs[i]);
                pthread_join(tids[i], NULL);
                pthread_attr_destroy(&attr[i]);
            }
            pthread_join(tids[nthreads], NULL);
            pthread_attr_destroy(&attr[nthreads]); 

            // number of cycles reported as skipped by the main thread should be at most
            // equal to the total number of cycles detected by all the helper threads
            // TODO: this seems to fail consistently for nthreads=3 -- not anymore!
            assert( ht_total_cycles >= cycles_skipped );

            //printf("%s\n", ( ht_total_cycles >= cycles_skipped ) ? "ALL GOOD" : "HMM..." );

            double hz = timer_read_hz();
            fprintf(stdout, "cycles:%lf  freq:%.0lf seconds:%lf  ", 
                            timer_total(&tim), hz, timer_total(&tim)/hz);
            
            // Print algorithm results
            weight_t msf_weight = 0.0;
            msf_edge_count = 0;

            for ( e = 0; e < el->nedges; e++ ){
                if ( edge_color_main[e] == MSF_EDGE ) {
                    msf_weight += el->edge_array[e].weight;
                    msf_edge_count++;
                }
            }
            fprintf(stdout, " msf_weight:%.0f", msf_weight);
            fprintf(stdout, " msf_edges:%d", msf_edge_count);
            fprintf(stdout, " cycles_main:%u", cycles_main);
            fprintf(stdout, " cycles_helper:%u", ht_total_cycles);
            fprintf(stdout, " cycles_skipped:%u\n", cycles_skipped);

            assert( msf_edge_count+cycles_main+cycles_skipped == el->nedges );

            // clean-up things
            pthread_barrier_destroy(&bar);
            free(tids);
            free(targs);
            free(attr);

            kruskal_destroy(al, array, edge_membership);
            kruskal_helper_destroy(edge_color_main, edge_color_helper);
        }
    }

    FILE * fd = fopen("statfile", "w");
    for ( i = 0; i < iterations; i++ ) {
        for ( j = 0; j < numrows; j++ ) {
            for ( k = 0; k < NUM_DATA_POINTS; k++ )
                fprintf(fd, "%u ", thread_stats[i][j][k]);
            fprintf(fd, "\n");
        }
        fprintf(fd, "\n");
    }
    fclose(fd);

    printf("iter = %d, numrows = %d, datap = %d\n", iterations, numrows, NUM_DATA_POINTS);
    for ( i = 0; i < iterations; i++ ) {
        for ( j = 0; j < numrows; j++ )
            free(thread_stats[i][j]);
        free(thread_stats[i]);
    }
    free(thread_stats);

    free(cycles_so_far);
    
    procmap_destroy(pi); 
    
    edgelist_destroy(el);
    adjlist_destroy(al);

    return 0;
}
