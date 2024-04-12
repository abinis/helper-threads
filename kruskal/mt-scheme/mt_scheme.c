#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <omp.h>

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
#endif

#include "mt_scheme.h"
#include "graph/edgelist.h"
#include "disjoint_sets/union_find_array.h"
#include "filter-kruskal/filter_kruskal.h"
#include "heap/heap.h"
#include "dgal-quicksort/quicksort.h"
#include "cpp_functions/cpp_psort.h"

extern pthread_barrier_t bar;
extern edgelist_t *el;
extern union_find_node_t *array;

//extern int nthr; //<- npthr might be more useful
extern int n_main_thr;
extern int n_helper_thr;
extern edge_t **msf;
extern unsigned int msf_edges;

int main_finished;
//long total_cycles_found = 0;

inline void kruskal_ht_scheme_init(void)
{
    array = union_find_array_init(el->nvertices);
    // at most nvertices-1 edges in the MSF :)
    msf = (edge_t**)malloc((el->nvertices-1)*sizeof(edge_t*));
    if ( !msf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
}

void kruskal_ht_scheme_destroy(void)
{
    free(array);
    free(msf);
}

void * kruskal_ht_scheme(void *args)
{
    targs_t *thread_args = (targs_t*)args;
    int id = thread_args->id;
    long from = thread_args->from[id];
    long to = thread_args->to[id];
    int thread_type = thread_args->type;

    #ifdef PROFILE
    double hz;
    #endif

    long i;
    edge_t *pe;
    int set1, set2;

    // for convenience
    edge_t *edge_array = el->edge_array;

    //printf("tid#%d [%ld,%ld)\n",id,from,to);

    // Code for the Main Thread
    if ( thread_type == MAIN_THR ) {

    #ifdef PROFILE
       tsctimer_t mtim;
    #endif

        main_finished = 0;

    #ifdef PROFILE
        timer_clear(&mtim);
        timer_start(&mtim);
    #endif
        pthread_barrier_wait(&bar);
    #ifdef PROFILE
        timer_stop(&mtim);
        hz = timer_read_hz();
        fprintf(stdout, "main 1st bar wait      cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        timer_total(&mtim),
                        timer_total(&mtim) / hz,
                        hz );
    #endif
        //printf("tid#%d after 1st barrier :)\n",id);
        // Phase 1 --> create MSF
    #ifdef PROFILE
        timer_clear(&mtim);
        timer_start(&mtim);
    #endif
    #ifdef CONCURRENT_MAIN
        if ( n_main_thr > 1 ) {
            //printf("n_main_thr=%d\n",n_main_thr);
            omp_set_num_threads(n_main_thr);
            fnode_array_global = array;
            filter_kruskal_concurrent_rec(el,/*array,*/from,to,msf,&msf_edges);
        } else
            filter_kruskal_rec(el,array,from,to,msf,&msf_edges);
    #else
        filter_kruskal_rec(el,array,from,to,msf,&msf_edges);
    #endif
    #ifdef PROFILE
        timer_stop(&mtim);
        hz = timer_read_hz();
        fprintf(stdout, "main ph1: left krusk   cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        timer_total(&mtim),
                        timer_total(&mtim) / hz,
                        hz );
    #endif

        main_finished = 1;

    //#ifdef PROFILE
    //    timer_clear(&mtim);
    //    timer_start(&mtim);
    //#endif
    //    pthread_barrier_wait(&bar);
    //#ifdef PROFILE
    //    timer_stop(&mtim);
    //    hz = timer_read_hz();
    //    fprintf(stdout, "main 1st bar wait      cycles:%18.2lf seconds:%10lf freq:%lf\n", 
    //                    timer_total(&mtim),
    //                    timer_total(&mtim) / hz,
    //                    hz );
    //#endif

    #ifdef PROFILE
        timer_clear(&mtim);
        timer_start(&mtim);
    #endif
        pthread_barrier_wait(&bar);
    #ifdef PROFILE
        timer_stop(&mtim);
        hz = timer_read_hz();
        fprintf(stdout, "main 2nd bar wait      cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        timer_total(&mtim),
                        timer_total(&mtim) / hz,
                        hz );
    #endif

        // Phase 2 --> heap!
    #ifdef PROFILE
        timer_clear(&mtim);
        timer_start(&mtim);
    #endif
        heap_aug_t *hp = heap_aug_create_empty(n_helper_thr, edge_compare);
//TODO need nthr, and from[t], e[t] for every thread!
        long edges_left = 0;
        int t;
        for ( t = 0; t < n_helper_thr; t++ ) {
            edges_left += thread_args->e[t+1];
            if ( thread_args->e[t+1] > 0 )
                heap_aug_add(hp, &(edge_array[thread_args->from[t+1]]),
                                &(edge_array[thread_args->from[t+1]+thread_args->e[t+1]-1]));
        }
        //printf("ht_region = %ld\n", el->nedges-to);
        //printf("total_cycles_found = %ld\n", total_cycles_found);
        //printf("edges_left = %ld\n", edges_left);
        //assert( el->nedges-to == total_cycles_found+edges_left );

        heap_aug_construct(hp);

        while ( !heap_aug_is_empty(hp) ) {

            edge_t *min = (edge_t*)heap_aug_peek_min(hp);

            set1 = union_find_array_find(array, min->vertex1);
            set2 = union_find_array_find(array, min->vertex2);

            if ( set1 != set2 ) {
                union_find_array_union(array, set1, set2);
                msf[msf_edges] = min;
                msf_edges++;
            }

            edge_t *next = (edge_t*)(min+1);
            heap_aug_increase_root_key(hp, next);
        }

        heap_aug_destroy(hp);
    #ifdef PROFILE
        timer_stop(&mtim);
        hz = timer_read_hz();
        fprintf(stdout, "main ph3: heap trav    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        timer_total(&mtim),
                        timer_total(&mtim) / hz,
                        hz );
    #endif

    // Helper Threads
    } else if ( thread_type == HELPER_THR ) {

        // 0. compute working bounds :)

        //long cf = 0;

    #ifdef PROFILE
        tsctimer_t htim;
        timer_clear(&htim);
        timer_start(&htim);
    #endif
        pthread_barrier_wait(&bar);
    #ifdef PROFILE
        timer_stop(&htim);
        hz = timer_read_hz();
        fprintf(stderr, "ht#%-2d 1st barrier wait cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        id,
                        timer_total(&htim),
                        timer_total(&htim) / hz,
                        hz );
    #endif

    //#ifdef PROFILE
    //    timer_clear(&htim);
    //    timer_start(&htim);
    //#endif
        //printf("tid#%d after 1st barrier :)\n",id);

        // 1. backward pass --> find cycles!
#ifndef MARK_AND_PUSH
        // in this case, use a (local) array to mark the cycles...
        char *edge_membership = calloc(to-from, sizeof(char));
        if ( !edge_membership) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
#else
        long cyc_pos = to-1;
#endif

        int loop_count = 0;
        // loop (perform backward/filtering passes) until a condition is met
        while ( 1 ) {
    #ifdef PROFILE
        timer_clear(&htim);
        timer_start(&htim);
    #endif
            // placeholder condition!
            if ( main_finished )
                break;

    #ifndef MARK_AND_PUSH
            for ( i = to-1; i >= from; i-- ) {
    #else
            for ( i = cyc_pos; i >= from; i-- ) {
    #endif
                //if ( (cyc_pos-from) % 1000 == 0 )
                //    if ( main_finished )
                //        break;
    
    #ifndef MARK_AND_PUSH
                if ( edge_membership[i-from] == 1 )
                    continue;
    #endif
                pe = &(edge_array[i]);
    
                // reminder: hts don't perform path compression :)
                set1 = union_find_array_find_helper(array, pe->vertex1);
                set2 = union_find_array_find_helper(array, pe->vertex2);
    
                // cycle found, mark the edge! -- or push it ;)
                if ( set1 == set2 ) {
    //TODO
    #ifndef MARK_AND_PUSH
                    edge_membership[i-from] = 1;
    #else
                    //swap_edge(&edge_array[cyc_pos--],pe); 
                    edge_array[i] = edge_array[cyc_pos--];
    #endif
                    //cf++;
                    //thread_args->cycles_found++;
                    //__sync_fetch_and_add(&total_cycles_found,1);
                }
            
            }
            loop_count++;
    #ifdef PROFILE
        timer_stop(&htim);
        hz = timer_read_hz();
        fprintf(stderr, "ht#%-2d loop#%-2d          cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        id,
                        loop_count,
                        timer_total(&htim),
                        timer_total(&htim) / hz,
                        hz );
    #endif
        }
    //#ifdef PROFILE
    //    timer_stop(&htim);
    //    hz = timer_read_hz();
    //    fprintf(stderr, "ht#%2d first %2d loops   cycles:%18.2lf seconds:%10lf freq:%lf\n", 
    //                    id,
    //                    loop_count,
    //                    timer_total(&htim),
    //                    timer_total(&htim) / hz,
    //                    hz );
    //#endif
        // loop once more!
    #ifdef PROFILE
        timer_clear(&htim);
        timer_start(&htim);
    #endif
    #ifndef MARK_AND_PUSH
            for ( i = to-1; i >= from; i-- ) {
    #else
            for ( i = cyc_pos; i >= from; i-- ) {
    #endif
    
    #ifndef MARK_AND_PUSH
                if ( edge_membership[i-from] == 1 )
                    continue;
    #endif
                pe = &(edge_array[i]);
    
                // reminder: hts don't perform path compression :)
                set1 = union_find_array_find_helper(array, pe->vertex1);
                set2 = union_find_array_find_helper(array, pe->vertex2);
    
                // cycle found, mark the edge! -- or push it ;)
                if ( set1 == set2 ) {
    //TODO
    #ifndef MARK_AND_PUSH
                    edge_membership[i-from] = 1;
    #else
                    //swap_edge(&edge_array[cyc_pos--],pe); 
                    edge_array[i] = edge_array[cyc_pos--];
    #endif
                    //cf++;
                    //thread_args->cycles_found++;
                    //__sync_fetch_and_add(&total_cycles_found,1);
                }
            
            }
    #ifdef PROFILE
        timer_stop(&htim);
        hz = timer_read_hz();
        fprintf(stderr, "ht#%-2d final loop!      cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        id,
                        timer_total(&htim),
                        timer_total(&htim) / hz,
                        hz );
    #endif

#ifndef MARK_AND_PUSH
        // 2. forward pass --> compact :)
        long e = from;

        for ( i = from; i < to; i++ ) {
            if ( edge_membership[i-from] == 0 ) {
                edge_array[e++] = edge_array[i];
            }
        }
        e = e - from;
#else
        // no need to if we've already pushed cycles towards the end :)
        long e = cyc_pos + 1 - from;
#endif
        //assert( e == to-from-cf );
        //assert( e == to-from-thread_args->cycles_found );
 
        // 3. sort remaining edges :)
    #ifdef PROFILE
        timer_clear(&htim);
        timer_start(&htim);
    #endif
#ifdef CPP_SORT
        cpp_sort_edge_arr(edge_array+from, e, 1);
#else
        qsort(edge_array+from, e, sizeof(edge_t), edge_compare);
#endif
    #ifdef PROFILE
        timer_stop(&htim);
        hz = timer_read_hz();
        fprintf(stderr, "ht#%-2d sort chunk :)    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        id,
                        timer_total(&htim),
                        timer_total(&htim) / hz,
                        hz );
    #endif

        // in any case, update e in targs so that main thr can heap-traverse :)
        thread_args->e[id] = e;
        //thread_args->e[id] = from+1;

    #ifdef PROFILE
        timer_clear(&htim);
        timer_start(&htim);
    #endif

        pthread_barrier_wait(&bar);

    #ifdef PROFILE
        timer_stop(&htim);
        hz = timer_read_hz();
        fprintf(stderr, "ht#%-2d 2nd barrier wait cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        id,
                        timer_total(&htim),
                        timer_total(&htim) / hz,
                        hz );
    #endif

#ifndef MARK_AND_PUSH
        free(edge_membership);
#endif

    //#ifdef PROFILE
    //    timer_stop(&htim);
    //    hz = timer_read_hz();
    //    fprintf(stdout, "ht#%2d total            cycles:%18.2lf seconds:%10lf freq:%lf\n", 
    //                    id,
    //                    timer_total(&htim),
    //                    timer_total(&htim) / hz,
    //                    hz );
    //#endif

    }

    pthread_exit(NULL);
}
