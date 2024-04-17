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
//#include "graph/adjlist.h"
//#include "disjoint_sets/union_find_array.h"
#include "machine/tsc_x86_64.h"
#include "mt_kruskal_cas.h"
#include "processor_map/processor_map.h"
//#define PROCMAP_H "processor_map/processor_map.h"
//#include PROCMAP_H
#include "util/util.h"
//#include "edge_mt_stats.h"

//not needed anymore
#define DATA_POINTS 100

//declared in mt_kruskal_cas.c
//how many cycles the main thread skipped thanks to helper thread work
unsigned int cycles_skipped = 0;
//how many cycles the man thread found
unsigned int cycles_main;
//how many cycles found by each ht
unsigned int *cycles_helper;
//unsigned int total_helper;

pthread_barrier_t bar;

#ifdef PROFILE
tsctimer_t tim;
tsctimer_t tim_main; // <- time spent by main thr in exclusive region...
tsctimer_t *tim_ht;  // <- ...and in (each) helper thread region :)
double hz;
#endif

//adjlist_stats_t stats;
edgelist_t *el;
//adjlist_t *al;
union_find_node_t *array;
union_find_node_t *s_array;
//unsigned int *edge_membership; 
char *edge_membership;
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

    for ( v = 0; v < _al->nvertices; v++ ) 
        _farray[v].rank += 0;
}
                             
static inline
void compare_serial_mt_accesses(unsigned int nedges,
                             char *edge_membership, 
                             char *edge_color_main)
{
    unsigned int e = 0, s_jumps = 0, mt_jumps = 0;
    // See how many jumps are needed in order to fully traverse
    // a marked edge_color/membership array :)
    while ( e < nedges ) {
        if ( edge_membership[e] < 0 ) {
            e += edge_membership[e]*(-1);
            //s_jumps++;
            //continue;
        }
        else
            e++;
        
        s_jumps++;
    }
    e = 0;
    while ( e < nedges ) {
        if ( edge_color_main[e] < 0 ) {
            e += edge_color_main[e]*(-1);
            //mt_jumps++;
            //continue;
        }
        else
            e++;
        
        mt_jumps++;
    }
    fprintf(stdout, "#accesses, serial vs mt: %u %u\n", s_jumps, mt_jumps);
}

// Actually, make_copy of any (void*) array :P
void * edge_membership_make_copy(unsigned long int len, size_t sz, void *arr)
{
    void *ret = calloc(len, sz);
    if (!ret) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    memcpy(ret, arr, len*sz);
    
    return ret;
}

int msf_edges_check(edgelist_t *el, char *edge_membership, char *copy)
{
    assert(el);
    assert(edge_membership);
    assert(copy);

    long int e;
    for ( e = 0; e < el->nedges; e++ ) {
        //printf("%d %d\n", edge_membership[e], copy[e]);
        // *both* must be 0 or 1 !
        if ( (edge_membership[e] == 1) ^ (copy[e] == 1) )
            return 0;
    }

    return 1;
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
        msf_edge_count;
    weight_t msf_weight;
    char graphfile[256];
    char mapping_string[4][4] = {"cpt",
                                 "pct",
                                 "tcp",
                                 "ctp"};

    if ( argc < 4 ) {
        fprintf(stderr, "Usage: %s <graphfile> <maxthreads> "
                        "<mapping= 0:cpt, 1:pct, 2:tcp, 3:ctp> [k] [iterations(default=1)] [partial run datapoints(default=1/off)]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    sprintf(graphfile, "%s", argv[1]);
    maxthreads = atoi(argv[2]);
    mapping = atoi(argv[3]);

    printf("maxthreads=%d\n", maxthreads);

    // main thread works in [0,k*nedges),
    // helper threads in (1-k)*nedges :)
    double k;
    int k_set = 0;
    if ( argc > 4 ) {
        k = atof(argv[4]);
        if ( !(k > 0.0 && k < 1.0) ) {
            printf("argument 4 (k) should be in (0.0,1.0)!\n");
            exit(EXIT_FAILURE);
        }
        k_set = 1;
    }
    if ( argc > 5 ) 
        iterations = atoi(argv[5]);

    // datapoints for partial run specified
    if ( argc > 6 )
        numdatapoints = atoi(argv[6]);

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
    } else if ( mapping == 2 ) {
        fprintf(stdout, "Thread fill order: threads>cores>packages\n");
        for ( p = pi->num_packages-1; p >= 0; p-- ) {
            for ( c = 0; c < pi->num_cores_per_package; c++ ) {
                for ( t = 0; t < pi->num_threads_per_core; t++ ) {
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
    } else if ( mapping == 3 ) {
        fprintf(stdout, "Thread fill order: cores>threads>packages\n");
        for ( p = pi->num_packages-1; p >= 0; p-- ) {
            for ( t = 0; t < pi->num_threads_per_core; t++ ) {
                for ( c = 0; c < pi->num_cores_per_package; c++ ) {
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

#ifdef PROFILE
    if ( maxthreads > 1 ) {
        tim_ht = (tsctimer_t*)malloc_safe( (maxthreads-1) * sizeof(tsctimer_t) );
    }
#endif

    char *inp_method;
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    // Init adjacency list
    //adjlist_init_stats(&stats);
    is_undirected = 0; // <- was 1
    //al = adjlist_read(graphfile, &stats, is_undirected);
    //el = edgelist_read(graphfile, is_undirected, 0);
    el = edgelist_choose_input_method(graphfile,is_undirected,0,&inp_method);
#ifdef PROFILE
    timer_stop(&tim);
    fprintf(stdout, "Read graph\n\n");
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_%s        cycles:%lu  freq:%.0lf seconds:%lf\n", 
                    inp_method,(uint64_t)timer_total(&tim), hz, timer_total(&tim)/hz);
#endif

    free(inp_method);
    //printf("el->nedges = %d\n", el->nedges);
    //timer_clear(&tim);
    //timer_start(&tim);
    // Create edge list from adjacency list
    //el = edgelist_create(al);
    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stdout, "edgelist_create      cycles:%lf  freq:%.0lf seconds:%lf\n", 
    //                timer_total(&tim), hz, timer_total(&tim)/hz);

    edge_t * unsorted_edges = (edge_t*)edge_membership_make_copy(el->nedges,
                                                                sizeof(edge_t),
                                                                el->edge_array);
 
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    // Sort edge list
    kruskal_sort_edges(el,/*maxthreads*/1); 
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal_sort_edges   cycles:%lu  freq:%.0lf seconds:%lf\n", 
                    (uint64_t)timer_total(&tim), hz, timer_total(&tim)/hz);
#endif

    fprintf(stdout, "\n");

    // Run serial kruskal for comparison :)
    fprintf(stdout, "Running serial kruskal + oracle...\n");
    kruskal_init_char(el, /*al,*/ &s_array, &edge_membership);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    kruskal_char(el, s_array, edge_membership);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal_char         cycles:%lu  freq:%.0lf seconds:%lf\n", 
                    (uint64_t)timer_total(&tim), hz, timer_total(&tim)/hz);
#endif
    unsigned int msf_edge_count_check = kruskal_get_msf_edge_count_char(el, edge_membership);
    weight_t msf_weight_check = kruskal_get_msf_weight_char(el, edge_membership);
    printf(" msf_edge_count_check = %u\n", msf_edge_count_check);
    printf("msf_edge_weight_check = %f\n", msf_weight_check);

    char *copy_char = (char*)edge_membership_make_copy(el->nedges,
                                                    sizeof(char),
                                                    edge_membership);

    kruskal_oracle_jump_prepare_char(el, s_array, copy_char);
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    kruskal_oracle_jump_char(el, s_array, copy_char);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "oracle_jump_char     cycles:%lu  freq:%.0lf seconds:%lf\n", 
                    (uint64_t)timer_total(&tim), hz, timer_total(&tim)/hz);
#endif

    assert( (msf_edge_count_check == kruskal_get_msf_edge_count_char(el,copy_char)) );
    assert(msf_edges_check(el,edge_membership,copy_char));
    //redunant, but better be sure :P
    assert( (msf_weight_check == kruskal_get_msf_weight_char(el,copy_char)) );

    free(copy_char);

    fprintf(stdout, "...done!\n");

    free(el->edge_array);
    el->edge_array = unsorted_edges;

    cycles_helper = calloc(maxthreads, sizeof(unsigned int));


    //mt_stats_t *mt_statsp = mt_stats_init(iterations, maxthreads, numdatapoints);

    // Run as many iterations as specified, default is one
    for ( iter = 0; iter < iterations; iter++ ) {
        // Run MT algorithm for different thread numbers
        for ( nthreads = 1; nthreads <= maxthreads; nthreads++ ) {
            // First, sort edges using all availabe threads :)
            // make an unsorted copy of the edge array...
            edge_t * edge_array_copy = (edge_t*)edge_membership_make_copy(el->nedges,
                                                               sizeof(edge_t),
                                                               el->edge_array);
            // temporarily set the calling thread to run on all available CPUs...
            cpu_set_t cpumask;
            CPU_ZERO(&cpumask);
            int cpunum;
            for ( cpunum = 0; cpunum < nthreads; cpunum++ )
                CPU_OR(&cpumask,&cpumask,&cpusets[cpunum]);
            sched_setaffinity(getpid(), sizeof(cpusets[0]), &cpumask);
            // sort edges...
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
            kruskal_sort_edges(el,nthreads);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal_sort_edges w/%2d thr   cycles:%lu  freq:%.0lf seconds:%lf\n", 
                    nthreads, (uint64_t)timer_total(&tim), hz, timer_total(&tim)/hz);
#endif
            // finally, revert to the initial cpu set!
            sched_setaffinity(getpid(), sizeof(cpusets[0]), &cpusets[0]);

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
                                nthreads, graphfile, mapping_string[mapping]);
  
                // Perform initializations 
                //kruskal_init(el, al, &array, &edge_membership);
                // Create and initialize forest nodes
                array = union_find_array_init(el->nvertices);
                kruskal_helper_init(el, &edge_color_main/*, &edge_color_helper*/);
                
                flush_caches(pi->num_cpus, llc_bytes);

                // Clear array where cycles found by helper threads are stored!
                memset(cycles_helper, 0, maxthreads*sizeof(unsigned int));

                // Allocate thread structures 
                tids = (pthread_t*)malloc_safe( nthreads * sizeof(pthread_t) );
                targs = (targs_t*)malloc_safe( nthreads * sizeof(targs_t)); 
                int *begin = (int*)malloc_safe( nthreads * sizeof(int) );
                int *end = (int*)malloc_safe( nthreads * sizeof(int) );
                attr = (pthread_attr_t*)malloc_safe( nthreads * sizeof(pthread_attr_t)); 
                pthread_barrier_init(&bar, NULL, nthreads);

            #ifdef PROFILE
                timer_clear(&tim);
                timer_clear(&tim_main);
                if ( nthreads > 1 ) {
                    for ( i = 0; i < nthreads-1; i++ )
                        timer_clear(&tim_ht[i]);
                }
            #endif

                // Create threads
                //assert(el->nedges > nthreads);
                assert(until > nthreads);
                // Helper threads get to work on the entire input,
                // even in a partial run! 

                // default value for k
                if ( !k_set )
                    k = (double)1.0 / (double)nthreads;
                //printf("k=%lf ", k);
                int ht_offset = el->nedges*k;
                int ht_num_edges = el->nedges-ht_offset;
                //printf("ht_num_edges = %d\n ht_offset = %d\n", ht_num_edges, ht_offset);
                //assert(ht_num_edges+ht_offset == el->nedges);
                int divide_by = ( nthreads > 1 ) ? nthreads-1 : nthreads ;
                int chunk_size = ht_num_edges/*el->nedges*/ / divide_by ;
                //int chunk_size = until / nthreads;
                // distribute remainder so that last helper thread
                // gets to examine up to the final edge (formerly,
                // it went up to nedges-remainder :)
                int rem = ht_num_edges/*el->nedges*/ % divide_by ;
                //int rem = until % nthreads;
                // also, first helper thread now *may* get to work on
                // [0, chunk_size), i.e. starts from the same spot
                // as the main thread TODO
                //int plusone = ( rem-- > 0 );
                //int plusone_prev = plusone;
                
                for ( i = 0; i < nthreads; i++ ) {
                    if ( i < 1 ) {
                        targs[i].type = MAIN_THR;
                        /*targs[i].*/begin[i] = 0;
                        /*targs[i].*/end[i] = ht_offset/*until*/;
                    } else {
                        targs[i].type = HELPER_THR;
                        /*targs[i].*/end[i] = ht_offset + (( i-1 <= rem ) ? (i-1)*(chunk_size+1) : rem*(chunk_size+1)+(i-1-rem)*chunk_size);
                        //plusone = ( rem-- > 0 );
                        int plusone = ( i-1 <= (rem-1) );
                        //printf("i = %d, rem = %d, i <= (rem-1) -> %d, plusone = %d\n", i, rem, i<=(rem-1), plusone);
                        /*targs[i].*/begin[i] = /*targs[i].*/end[i] + chunk_size + plusone;
                        //plusone_prev = plusone;
                    }
                    //printf("thr:%d [%u,%u) of %u\n", i, targs->end[i], targs->begin[i], until);
                    targs[i].id = i;
                    // every thread gets to know the bounds of every other :)
                    targs[i].begin = begin;
                    targs[i].end = end;
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
                        for ( edge = /*targs[i].*/begin[i]; edge < /*targs[i].*/end[i]; edge++ )
                            if ( edge_color_main[edge] == CYCLE_EDGE_MAIN )
                                cycles_main++;
                    }
                    else {
                        for ( edge = /*targs[i].*/end[i]; edge < /*targs[i].*/begin[i]; edge++ )
                            if ( edge_color_main[edge] < 0 ) {
                                cycles_helper[i]++;
                                ht_total_cycles++;
                            }
                    }

                    /*
                    int min,max;
                    min = targs[i].end;
                    max = targs[i].begin;
                    if (i == 0) {
                        min = targs[i].begin;
                        max = targs[i].end;
                    }

                    fprintf(stdout, "tid=%d[%d,%d)loops=%d\n", targs[i].id, min, max, targs[i].ht_loop_count);
                    */

                    pthread_attr_destroy(&attr[i]);
                }

                // number of cycles reported as skipped by the main thread should be at most
                // equal to the total number of cycles detected by all the helper threads
                //assert( ht_total_cycles >= cycles_skipped );

            #ifdef PROFILE
                hz = timer_read_hz();
                uint64_t tim_ht_tot = 0;
                for ( i = 1; i < nthreads; i++ )
                    tim_ht_tot += (uint64_t)timer_total(&tim_ht[i-1]);
                fprintf(stdout, "cycles:%lu", (uint64_t)timer_total(&tim));
                fprintf(stdout, "{%lu", (uint64_t)timer_total(&tim_main));
                if ( nthreads > 1 ) {
                    fprintf(stdout, "|%lu{", tim_ht_tot);
                    for ( i = 1; i < nthreads; i++ )
                        fprintf(stdout, "%lu%c", (uint64_t)timer_total(&tim_ht[i-1]), ( i == nthreads-1 ) ? '}' : ',' );
                }
                fprintf(stdout, "}");
                fprintf(stdout, " freq:%.0lf", hz);
                fprintf(stdout, " seconds:%lf", timer_total(&tim)/hz);
                fprintf(stdout, "{%lf", timer_total(&tim_main)/hz);
                if ( nthreads > 1 ) {
                    fprintf(stdout, "|%lf{", tim_ht_tot/hz);
                    for ( i = 1; i < nthreads; i++ )
                        fprintf(stdout, "%lf%c", timer_total(&tim_ht[i-1])/hz, ( i == nthreads-1 ) ? '}' : ',' );
                }
                fprintf(stdout, "}");
                fprintf(stdout, " batch#%d:[0,%u) " , datapoint, until);
                
            #endif
                
                // Print algorithm results
                //if ( until == el->nedges ) {
                msf_weight = 0.0;
                msf_edge_count = 0;
                //cycles_main = 0;

                /*printf("\n");
                for ( e = 0; e < until; e++ ) {
                    printf("(%3d)%4d", e, edge_color_main[e]);
                    if ( (e+1) % 16 == 0 )
                        printf("\n");
                    else
                        printf(" ");
                }
                printf("\n");*/

                /*
                char filename[32];
                memset(filename, 0, 32);
                snprintf(filename, 32, "msf_edge_set_%d", nthreads);
                FILE* edge_set = fopen(filename, "w");
                */
                for ( e = 0; e < el->nedges; e++ ) {
                    if ( edge_color_main[e] == MSF_EDGE ) {
                        msf_weight += el->edge_array[e].weight;
                        msf_edge_count++;
                        //fprintf(edge_set, "%d\n", e);
                    } /*else if ( edge_color_main[e] == CYCLE_EDGE_MAIN ) {
                        cycles_main++;
                        cycles_helper[edge_color_main[e]-1]++;
                    }
                    else {
                        cycles_helper[edge_color_main[e]-1]++;
                        ht_total_cycles++;
                    }*/
                }
                //fclose(edge_set);
                
                fprintf(stdout, " msf_weight:%.0f", msf_weight);
                fprintf(stdout, " msf_edges:%d", msf_edge_count);
                //}

                fprintf(stdout, " cycles_main:%u", cycles_main);
                fprintf(stdout, " cycles_helper:%u", ht_total_cycles);
                int hthr;
                for ( hthr = 1; hthr < nthreads; hthr++ )
                    fprintf(stdout, " cycles_ht#%d:%u", hthr, cycles_helper[hthr]);
                fprintf(stdout, " cycles_skipped:%u\n", cycles_skipped);

                //assert( msf_edge_count+cycles_main+cycles_skipped == until );

                //mt_stats_record_fill(mt_statsp, iter, nthreads-1, datapoint,
                //                     until, timer_total(&tim)/hz, msf_edge_count,
                //                     cycles_main, ht_total_cycles, cycles_skipped);


                until = ( until+batch_size >= el->nedges ) ? el->nedges : until+batch_size;
                if ( remainder-- > 0 )
                    until += 1;

                //assert( msf_edge_count+cycles_main+cycles_skipped == el->nedges );

                // clean-up things
                pthread_barrier_destroy(&bar);
                free(tids);
                free(targs);
                free(begin);
                free(end);
                free(attr);

                //kruskal_destroy(al, array, edge_membership);
                union_find_array_destroy(array);

                compare_serial_mt_accesses(el->nedges, edge_membership, edge_color_main);
                
                kruskal_helper_destroy(edge_color_main/*, edge_color_helper*/);

                //TODO: fill-in stats (which stats? add some more?)
                //mt_stats[iter][nthreads][datapoint].
            }

            //mt_stats_partial_progress(mt_statsp, iter, nthreads, el->nedges, msf_edge_count_check);
            free(el->edge_array);
            el->edge_array = edge_array_copy;
        }
    }

    //mt_stats_destroy(mt_statsp);

    kruskal_destroy(/*al,*/ s_array, edge_membership);

    free(cycles_helper);
    
    procmap_destroy(pi); 
    
    edgelist_destroy(el);
    //adjlist_destroy(al);

    return 0;
}
