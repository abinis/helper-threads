#define _GNU_SOURCE

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mt_kruskal.h"

#include "disjoint_sets/union_find_array.h"
#include "graph/adjlist.h"
#include "graph/graph.h"
#include "graph/edgelist.h"
#include "machine/tsc_x86_64.h"

int main_finished;

extern int cycles_skipped;

extern pthread_barrier_t bar;
extern tsctimer_t tim;
extern edgelist_t *el;
extern union_find_node_t *array;
extern char *edge_color_main;
extern char *edge_color_helper;

/**
 * Initialize Kruskal-HT structures
 * @param el pointer to sorted edge list 
 * @param edge_color_main pointer to per-edge color array of main thread
 * @param edge_color_helper pointer to per-edge color array of helper threads
 */
void kruskal_helper_init(edgelist_t *el, 
                         char **edge_color_main, 
                         char **edge_color_helper)
{
    unsigned int e;

    *edge_color_main = (char*)malloc(el->nedges * sizeof(char));
    if ( ! *edge_color_main ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    *edge_color_helper = (char*)malloc(el->nedges * sizeof(char));
    if ( ! *edge_color_helper ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    for ( e = 0; e < el->nedges; e++ )
        (*edge_color_main)[e] = (*edge_color_helper)[e] = 0;
} 

/**
 * Destroy Kruskal-HT structures
 * @param edge_color_main per-edge color array of main thread
 * @param edge_color_helper per-edge color array of helper threads
 *  
 */
void kruskal_helper_destroy(char *edge_color_main, 
                             char *edge_color_helper)
{
    free(edge_color_main);
    free(edge_color_helper);
}

/**
 * Find-set function for helper threads. Operates as the original 
 * find-set, without compressing the path
 * @param array the ancestor array maintained by the union-find data structure
 * @param index the node whose set we want to find
 * @return the root of the subtree where the node belongs to
 */ 
static inline int find_set_helper(union_find_node_t* array, int index) 
{
    int root = index;
    while ( array[root].parent != root )
        root = array[root].parent;
        
    return root;
}

/**
 * Kruskal-HT thread function
 */ 
void *kruskal_ht(void *args)
{
    targs_t *thread_args = (targs_t*)args;
    int id = thread_args->id;
    int begin = thread_args->begin;
    int end = thread_args->end;
    int thread_type = thread_args->type;
    unsigned int i;
    // changing type to int since we're using array impl of union-find
    int set1, set2;
    edge_t *pe;

    assert(array);

    // Code for the Main Thread
    if ( thread_type == MAIN_THR ) {
        
        main_finished = 0;
    
        pthread_barrier_wait(&bar);
        timer_start(&tim);

        for ( i = begin; i < end; i++ ) {
            pe = &(el->edge_array[i]);

            if( edge_color_helper[i] != 0 ) {
                // see how many cycles we skip thanks to the work of the helper threads :)
                cycles_skipped++;
                continue;
            }
            else {
                set1 = union_find_array_find(array, pe->vertex1);
                set2 = union_find_array_find(array, pe->vertex2);
                
                if ( set1 != set2 ) {
                    union_find_array_union(array, set1, set2);
                    edge_color_main[i] = MSF_EDGE;
                } else 
                    edge_color_main[i] = CYCLE_EDGE_MAIN;
            }
        }   
        main_finished = 1;
        timer_stop(&tim);
        
        pthread_barrier_wait(&bar);
    
    // Helper Threads
    } else if ( thread_type == HELPER_THR ) {
        
        pthread_barrier_wait(&bar);
        
        i = begin - 1;
        while ( i > end ) {

            // upon reaching end, recycle, or exit if main has finished
            if ( i == end + 1 ) {
                i = begin - 1;
                if ( main_finished ) break;
            }

            if ( edge_color_helper[i] == 0 ) {
                pe = &(el->edge_array[i]);
                set1 = find_set_helper(array, pe->vertex1);
                set2 = find_set_helper(array, pe->vertex2);
                
                if ( set1 == set2 ) 
                    edge_color_helper[i] = id + 1;

            } else if ( edge_color_main[i] != 0 ) {
                printf("thr %d cycle caused by edge #%d of %d (%.2f%%) %d(%d,%d) already detected by main thread! breaking...\n", id, begin-i, begin-end, (float)100*(begin-i)/(begin-end), i, begin, end);
                break;
            }

            i--;
        }
        
        pthread_barrier_wait(&bar);

    } // end of helper code 

    pthread_exit(NULL);
}
