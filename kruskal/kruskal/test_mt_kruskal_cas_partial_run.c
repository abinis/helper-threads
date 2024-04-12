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
#include "mt_kruskal_cas.h"
#include "processor_map/processor_map.h"
//#define PROCMAP_H "processor_map/processor_map.h"
//#include PROCMAP_H
#include "util/util.h"

//not needed anymore
#define DATA_POINTS 100

//declared in mt_kruskal_cas.c
//how many cycles the main thread skipped thanks to helper thread work
unsigned int cycles_skipped;
//how many cycles the man thread found
unsigned int cycles_main;
//how many cycles found by each ht
unsigned int *cycles_helper;
//unsigned int total_helper;

pthread_barrier_t bar;
tsctimer_t tim;

adjlist_stats_t stats;
edgelist_t *el;
adjlist_t *al;
union_find_node_t *array;
unsigned int *edge_membership; 
char *edge_color_main;
//char *edge_color_helper;

int nthreads;

void touch_structures(edgelist_t *_el,
                             adjlist_t *_al,
                             union_find_node_t *_farray,
                             char *_edge_color_main/*,
                             char *_edge_color_helper*/)
{
    unsigned int e, v;

    for ( e = 0; e < _el->nedges; e++ ) {
        _edge_color_main[e] += 0;
        //_edge_color_helper[e] += 0;
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
    int p, c, t, i, e, iter, maxthreads, 
        is_undirected, mapping,
        iterations = 1,
        numdatapoints = 1,
        msf_edge_count = 0;
    char graphfile[256];

    if ( argc < 4 ) {
        fprintf(stderr, "Usage: %s <graphfile> <maxthreads> "
                        "<mapping= 0:cpt, 1:pct> [iterations(default=1)] [partial run datapoints(default=1/off)]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    sprintf(graphfile, "%s", argv[1]);
    maxthreads = atoi(argv[2]);
    mapping = atoi(argv[3]);

    if ( argc > 4 ) 
        iterations = atoi(argv[4]);

    // datapoints for partial run specified
    if ( argc > 5 )
        numdatapoints = atoi(argv[5]);

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

    cycles_helper = calloc(maxthreads, sizeof(unsigned int));

    // Run as many iterations as specified, default is one
    for ( iter = 0; iter < iterations; iter++ ) {
        // Run MT algorithm for different thread numbers
        for ( nthreads = 1; nthreads <= maxthreads; nthreads++ ) {
            // Run MT algorithm for first few edges
            unsigned int batch_size = el->nedges / numdatapoints;
            // distribute remainder across batches so we end up with exactly
            // numdatapoints (as many as were specified), as equidistant as
            // possible! :)
            int remainder = el->nedges % numdatapoints;
            
            unsigned int until = batch_size;
            if ( remainder-- > 0 )
                until += 1;
            
            int datapoint;
            //int maxdatapoints = ( el->nedges % numdatapoints == 0 ) ? numdatapoints : numdatapoints+1;

            for ( datapoint = 0; datapoint < numdatapoints; datapoint++ ) {
                fprintf(stdout, "nthreads:%d  graph:%s  mapping:%s  ", 
                                nthreads, graphfile, mapping == 0 ? "cpt":"pct");
  
                // Perform initializations 
                kruskal_init(el, al, &array, &edge_membership);
                kruskal_helper_init(el, &edge_color_main/*, &edge_color_helper*/);
                
                flush_caches(pi->num_cpus, llc_bytes);

                // Clear array where cycles found by helper threads are stored!
                memset(cycles_helper, 0, maxthreads*sizeof(unsigned int));

                // Allocate thread structures 
                tids = (pthread_t*)malloc_safe( nthreads * sizeof(pthread_t) );
                targs = (targs_t*)malloc_safe( nthreads * sizeof(targs_t)); 
                attr = (pthread_attr_t*)malloc_safe( nthreads * sizeof(pthread_attr_t)); 
                pthread_barrier_init(&bar, NULL, nthreads);

                timer_clear(&tim);

                // Create threads
                //assert(el->nedges > nthreads);
                assert(until > nthreads);
                //int chunk_size = el->nedges / nthreads;
                int chunk_size = until / nthreads;
                // distribute remainder so that last helper thread
                // gets to examine up to the final edge (formerly,
                // it went up to nedges-remainder :)
                int rem = until % nthreads;
                // also, first helper thread now *may* get to work on
                // [0, chunk_size), i.e. starts from the same spot
                // as the main thread TODO <- DONE in "overlap"
                //int plusone = ( rem-- > 0 );
                //int plusone_prev = plusone;
                
                for ( i = 0; i < nthreads; i++ ) {
                    if ( i < 1 ) {
                        targs[i].type = MAIN_THR;
                        targs[i].begin = 0;
                        targs[i].end = until;
                    } else {
                        targs[i].type = HELPER_THR;
                        targs[i].end = ( i <= rem ) ? i*(chunk_size+1) : rem*(chunk_size+1)+(i-rem)*chunk_size;
                        //plusone = ( rem-- > 0 );
                        int plusone = ( i <= (rem-1) );
                        //printf("i = %d, rem = %d, i <= (rem-1) -> %d, plusone = %d\n", i, rem, i<=(rem-1), plusone);
                        targs[i].begin = targs[i].end + chunk_size + plusone;
                        //plusone_prev = plusone;
                    }
                    //printf("thr:%d [%u,%u) of %u\n", i, targs[i].end, targs[i].begin, until);
                    targs[i].id = i;
                    // extra args for keeping stats
                    targs[i].ht_loop_count = 0;
                    memset(targs[i].ht_cycles_per_loop, 0, TARGS_LOOP_ARR_SZ);
                    // TODO probably redundant
                    targs[i].p = targs[i].ht_cycles_per_loop;
                    targs[i].why = 0;

                    pthread_attr_init(&attr[i]);
                    pthread_attr_setaffinity_np(&attr[i], 
                                                sizeof(cpusets[i]), 
                                                &cpusets[i]);
                    pthread_create(&tids[i], &attr[i], kruskal_ht, (void*)&targs[i]);
                }

                // total cycles found by all helper threads
                unsigned int ht_total_cycles = 0;
                cycles_main = 0;
                for ( i = 0; i < nthreads; i++ ) {
                    //kruskal_helper_print_stats((void*)&targs[i]);
                    //ht_total_cycles += kruskal_helper_get_total_cycles((void*)&targs[i]);
                    pthread_join(tids[i], NULL);

                    int edge;
                    if ( i == 0 ) {
                        for ( edge = targs[i].begin; edge < targs[i].end; edge++ )
                            if ( edge_color_main[edge] == CYCLE_EDGE_MAIN )
                                cycles_main++;
                    }
                    else {
                        for ( edge = targs[i].end; edge < targs[i].begin; edge++ )
                            if ( edge_color_main[edge] == targs[i].id+1 ) {
                                cycles_helper[i]++;
                                ht_total_cycles++;
                            }
                    }

                    pthread_attr_destroy(&attr[i]);
                }

                // number of cycles reported as skipped by the main thread should be at most
                // equal to the total number of cycles detected by all the helper threads
                //assert( ht_total_cycles >= cycles_skipped );

                double hz = timer_read_hz();
                fprintf(stdout, "cycles:%lf  freq:%.0lf seconds:%lf  batch#%d:[0,%u) ", 
                                timer_total(&tim), hz, timer_total(&tim)/hz, datapoint, until);
                
                // Print algorithm results
                //if ( until == el->nedges ) {
                weight_t msf_weight = 0.0;
                msf_edge_count = 0;
                //cycles_main = 0;

                //printf("\n");
                //for ( e = 0; e < until; e++ ) {
                //    printf("%03d", edge_color_main[e]);
                //    if ( (e+1) % 8 == 0 )
                //        printf(" ");
                //    else
                //        printf(".");
                //}
                //printf("\n");

                for ( e = 0; e < until; e++ ){
                    if ( edge_color_main[e] == MSF_EDGE ) {
                        msf_weight += el->edge_array[e].weight;
                        msf_edge_count++;
                    } /*else if ( edge_color_main[e] == CYCLE_EDGE_MAIN ) {
                        cycles_main++;
                        cycles_helper[edge_color_main[e]-1]++;
                    }
                    else {
                        cycles_helper[edge_color_main[e]-1]++;
                        ht_total_cycles++;
                    }*/
                }
                
                fprintf(stdout, " msf_weight:%.0f", msf_weight);
                fprintf(stdout, " msf_edges:%d", msf_edge_count);
                //}

                fprintf(stdout, " cycles_main:%u", cycles_main);
                fprintf(stdout, " cycles_helper:%u", ht_total_cycles);
                int hthr;
                for ( hthr = 1; hthr < nthreads; hthr++ )
                    fprintf(stdout, " cycles_ht#%d:%u", hthr, cycles_helper[hthr]);
                fprintf(stdout, " cycles_skipped:%u\n", cycles_skipped);

                assert( msf_edge_count+cycles_main+cycles_skipped == until );

                until = ( until+batch_size >= el->nedges ) ? el->nedges : until+batch_size;
                if ( remainder-- > 0 )
                    until += 1;

                //assert( msf_edge_count+cycles_main+cycles_skipped == el->nedges );

                // clean-up things
                pthread_barrier_destroy(&bar);
                free(tids);
                free(targs);
                free(attr);

                kruskal_destroy(al, array, edge_membership);
                kruskal_helper_destroy(edge_color_main/*, edge_color_helper*/);
            }
        }
    }

    free(cycles_helper);
    
    procmap_destroy(pi); 
    
    edgelist_destroy(el);
    adjlist_destroy(al);

    return 0;
}
