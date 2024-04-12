#define _GNU_SOURCE

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mt_kruskal_cas.h"

#include "disjoint_sets/union_find_array.h"
#include "graph/adjlist.h"
#include "graph/graph.h"
#include "graph/edgelist.h"
#include "machine/tsc_x86_64.h"

int main_finished;

extern unsigned int cycles_skipped;
//extern unsigned int cycles_main;

extern pthread_barrier_t bar;
extern tsctimer_t tim;
extern edgelist_t *el;
extern union_find_node_t *array;
extern char *edge_color_main;
//extern char *edge_color_helper;

/**
 * Get total number of cycles found by a helper thread
 */
/*unsigned int kruskal_helper_get_total_cycles(void *targs)
{
    targs_t *args = (targs_t*)targs;
    char *log = args->ht_cycles_per_loop;

    char *ptr = log;
    int pos = 0;
    int loop;
    unsigned int cycles, total = 0;
    int ret;

    while ( ptr < log+TARGS_LOOP_ARR_SZ ) {
        ret = sscanf(ptr, "%d:%u %n", &loop, &cycles, &pos);
        if ( ret == 2 )
            total += cycles;
        else
            break;

        ptr += pos;
    }

    return total;
}
*/

/**
 * Print helper thread related stats
 */
/*void kruskal_helper_print_stats(void *targs)
{
    targs_t *args = (targs_t*)targs;
    fprintf(stdout, "htid:%d ", args->id);
    fprintf(stdout, "t:%u ", kruskal_helper_get_total_cycles(targs));
    fprintf(stdout, "%s", args->ht_cycles_per_loop);
    //TODO: multiple break reasons?
    fprintf(stdout, "%s\n", ( args->why == MAIN_FIN ) ? "main_fin" : "cycle_found");
}
*/

/**
 * Log cycles found by helper thread
 * @param id the helper thread id
 * @param log the buffer to write into
 * @param pos in which position
 * @param l the loop #
 * @param c how many cycles the ht found within l
 */
/*void kruskal_helper_log_cycles(int id, char *log, char **pos, int l, unsigned int c)
{
    char buffer[32];
    //memset(buffer, 0, 32);
    int about_to_write = sprintf(buffer, "%d:%u ", l, c);
    if ( (*pos)+about_to_write < log+TARGS_LOOP_ARR_SZ-1 )
        (*pos) += sprintf(*pos, "%d:%u ", l, c);
    else 
        fprintf(stderr, "thr %d ran out of cycles per loop reporting space! moving on...\n", id);
    
    //ptr += chars_written;
}
*/

/**
 * Initialize Kruskal-HT structures
 * @param el pointer to sorted edge list 
 * @param edge_color_main pointer to per-edge color array of main thread
 * @param edge_color_helper pointer to per-edge color array of helper threads
 */
void kruskal_helper_init(edgelist_t *el, 
                         char **edge_color_main/*, 
                         char **edge_color_helper*/)
{
    unsigned int e;

    *edge_color_main = (char*)malloc(el->nedges * sizeof(char));
    if ( ! *edge_color_main ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    /* We won't need that for this implementation!
    
    *edge_color_helper = (char*)malloc(el->nedges * sizeof(char));
    if ( ! *edge_color_helper ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    

    for ( e = 0; e < el->nedges; e++ )
        (*edge_color_main)[e] = (*edge_color_helper)[e] = 0;
    */

    for ( e = 0; e < el->nedges; e++ )
        (*edge_color_main)[e] = 0;
} 

/**
 * Destroy Kruskal-HT structures
 * @param edge_color_main per-edge color array of main thread
 *  
 */
void kruskal_helper_destroy(char *edge_color_main)
{
    free(edge_color_main);
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
    //int *ht_loop_count = &(thread_args->ht_loop_count);
    //char *log = thread_args->ht_cycles_per_loop;
    //char *pos = thread_args->p;
    //enum ht_break_reason *why = &(thread_args->why);
    // default to cycle found by main, since we don't check for that event
    // in the code below ;) TODO: for the time being, this won't happen, 
    // exactly for that reason (ht only breaks after main finished)
    //*why = CYCLE_FOUND_BY_MAIN;
    // how many cycles found by ht in its current looping through its edges
    //unsigned int ht_cycles_found = 0;
    //int chars_written = 0;

    unsigned int i;
    // changing type to int since we're using array impl of union-find
    int set1, set2;
    edge_t *pe;

    cycles_skipped = 0;
    //cycles_main = 0;

    //char buffer[32];
    //memset(buffer, 0, 32);
    //int ht_spins = 0;

    assert(array);

    // Code for the Main Thread
    if ( thread_type == MAIN_THR ) {
        
        main_finished = 0;
    
        pthread_barrier_wait(&bar);
        timer_start(&tim);

        for ( i = begin; i < end; i++ ) {
            pe = &(el->edge_array[i]);
            //printf("got handle to edge#%d, color: %d, vertex1: %d, vertex2: %d\n", i, pe->edge_color, pe->vertex1, pe->vertex2);

            if( edge_color_main[i] != 0 ) {
                // see how many cycles we skip thanks to the work of the helper threads :)
                cycles_skipped++;
                continue;
            }
            else {
                set1 = union_find_array_find(array, pe->vertex1);
                set2 = union_find_array_find(array, pe->vertex2);
                
                if ( set1 != set2 ) {
                    union_find_array_union(array, set1, set2);
                    //__atomic_val_compare_and_swap(&edge_color_main[i], 0, MSF_EDGE);
                    edge_color_main[i] = MSF_EDGE;
                } else {
                    //cycles_main++;
                    //__atomic_val_compare_and_swap(&edge_color_main[i], 0, CYCLE_EDGE_MAIN);
                    edge_color_main[i] = CYCLE_EDGE_MAIN;
                }
            }
        }   
        main_finished = 1;
        timer_stop(&tim);
        
        pthread_barrier_wait(&bar);
    
    // Helper Threads
    } else if ( thread_type == HELPER_THR ) {
        
        pthread_barrier_wait(&bar);
        
        i = begin - 1;
        while ( i >= end ) {

            // upon reaching end, recycle, or exit if main has finished
            // note: was end + 1
            if ( i == end ) {
                i = begin - 1;
                //(*ht_loop_count)++;
                // TODO: ptr out of bounds check
                //kruskal_helper_log_cycles(id, log, &pos, *ht_loop_count, ht_cycles_found);
                //int about_to_write = sprintf(buffer, "%d:%u ",(*ht_loop_count)+1, ht_cycles_found);
                //if ( ptr+chars_written < ptr+TARGS_LOOP_ARR_SZ )
                //    chars_written += sprintf(ptr+chars_written, "%d:%u ", (*ht_loop_count), ht_cycles_found);
                //else 
                //    fprintf(stderr, "thr %d ran out of cycles per loop reporting space! moving on...\n", id);
                
                //ptr += chars_written;
                //ht_cycles_found = 0;
                if ( main_finished ) {
                    //printf("thr %d reached main_finished after %d spins, breaking...\n", id, (*ht_loop_count));
                    //*why = MAIN_FIN;
                    break;
                }
            }

            // stay ahead of the main thread!
            if ( edge_color_main[i] == MSF_EDGE || edge_color_main[i] == CYCLE_EDGE_MAIN ) {
                end = i;
                if ( end == begin )
                    break;

                i = begin - 1;
                continue;
            }

            // Note: was _helper originally
            //if ( edge_color_main[i] == 0 ) {
                pe = &(el->edge_array[i]);
                set1 = find_set_helper(array, pe->vertex1);
                set2 = find_set_helper(array, pe->vertex2);
                
                if ( set1 == set2 ) {
                   /*ht_cycles_found += (*/ __sync_bool_compare_and_swap(&edge_color_main[i], 0, id + 1) /*) ? 1 : 0*/;
                      //printf("cas by thr %d for edge #%d of %d (%.2f%%) %d(%d,%d) failed! breaking...\n", id, begin-i, begin-end, (float)100*(begin-i)/(begin-end), i, begin, end);
                      //break;
                   //}
                }

            //} else if ( edge_color_main[i] != 0 ) 
            //    break;

            //if ( edge_color_main[i] != 0 ) {
            //    printf("thr %d cycle caused by edge #%d of %d (%.2f%%) %d(%d,%d) already detected by main thread! breaking...\n", id, begin-i, begin-end, (float)100*(begin-i)/(begin-end), i, begin, end);
            //    break;
            //}

            i--;
        }
        
        pthread_barrier_wait(&bar);

    } // end of helper code 

    pthread_exit(NULL);
}
