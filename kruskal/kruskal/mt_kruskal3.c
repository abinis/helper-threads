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

#include "disjoint_sets/union_find.h"
#include "graph/adjlist.h"
#include "graph/graph.h"
#include "graph/edgelist.h"
#include "machine/tsc_x86_64.h"

int main_finished;

extern pthread_barrier_t bar;
extern tsctimer_t tim, tim2, tim3, tim4, tim5, tim6, tim7;
extern edgelist_t *el;
extern forest_node_t **fnode_array;
extern int *edge_color_main;
extern int *loopcnt;

/**
 * Initialize Kruskal-HT structures
 * @param el pointer to sorted edge list 
 * @param edge_color_main pointer to per-edge color array of main thread
  */
void kruskal_helper_init2(edgelist_t *el, 
                         char **edge_color_main)
{
    unsigned int e;

    *edge_color_main = (char*)malloc(el->nedges * sizeof(char));
    if ( ! *edge_color_main ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }


    for ( e = 0; e < el->nedges; e++ )
        (*edge_color_main)[e] = 0;
} 

/**
 * Initialize Kruskal-HT structures
 * @param el pointer to sorted edge list 
 * @param edge_color_main pointer to per-edge color array of main thread
  */
void kruskal_helper_init3(edgelist_t *el, 
                         int **edge_color_main)
{
    unsigned int e;

    *edge_color_main = (int*)malloc(el->nedges * sizeof(int));
    if ( ! *edge_color_main ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }


    for ( e = 0; e < el->nedges; e++ )
        (*edge_color_main)[e] = 0;
}

/**
 * Destroy Kruskal-HT structures
 * @param edge_color_main per-edge color array of main thread
 *  
 */
void kruskal_helper_destroy2(char *edge_color_main)
{
    free(edge_color_main);
}

/**
 * Destroy Kruskal-HT structures
 * @param edge_color_main per-edge color array of main thread
 *  
 */
void kruskal_helper_destroy3(int *edge_color_main)
{
    free(edge_color_main);
}

/**
 * Find-set function for helper threads. Operates as the original 
 * find-set, without compressing the path
 * @param node the node whose set we want to find
 * @return the root of the subtree where the node belongs to
 */ 
static inline forest_node_t* find_set_helper(forest_node_t* node) 
{
    forest_node_t* root = node;
    while ( root->parent != NULL )
        root = root->parent;
        
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
    int negid;
    forest_node_t *set1, *set2;
    edge_t *pe;

    assert(fnode_array);

    // Code for the Main Thread
    if ( thread_type == MAIN_THR ) {
        
        main_finished = 0;
    
        pthread_barrier_wait(&bar);
        //timer_start(&tim4);

        for ( i = begin; i < end; i++ ) {
            pe = &(el->edge_array[i]);

            if( edge_color_main[i] < 0 ) {
                
                //printf("tasos %d",i);
                i = i + edge_color_main[i] * (-1) - 1;
                continue;
            }
            //else if ( edge_color_main[i] > 0 )
                //continue;  
            else if (edge_color_main[i] == 0 ){
                timer_start(&tim2);
                set1 = find_set(fnode_array[pe->vertex1]);
                set2 = find_set(fnode_array[pe->vertex2]);
                timer_stop(&tim2);
                
                if ( set1 != set2 ) {
                    //timer_start(&tim3);
                    union_sets(set1, set2);
                    //timer_stop(&tim3);
                    edge_color_main[i] = MSF_EDGE;
                } else 
                    edge_color_main[i] = CYCLE_EDGE_MAIN;
            }
        }   
        main_finished = 1;
        //timer_stop(&tim4);
        
        pthread_barrier_wait(&bar);
    
    // Helper Threads
    } else if ( thread_type == HELPER_THR ) {
        
        pthread_barrier_wait(&bar);
        
        i = begin - 1;
        negid = -1;
        while ( i > end ) {
            
            // upon reaching end, recycle, or exit if main has finished
            if ( i == end + 1 ) {
                i = begin - 1;
                negid = -1;
                //loopcnt[id]++;
                if ( main_finished ) break;
            }

            //if (loopcnt[id] % 3 < 2) {
                if ( edge_color_main[i] < 0) {    
                  __sync_val_compare_and_swap(edge_color_main + i , 0, negid);
                  negid--;
                }

                if ( edge_color_main[i] == 0 ) {      ////// !!!!!! changed edge_color_helper to edge_color_main !!!!!!
                    timer_start(&tim);
                    pe = &(el->edge_array[i]);
                    set1 = find_set_helper(fnode_array[pe->vertex1]);
                    set2 = find_set_helper(fnode_array[pe->vertex2]);
                    //timer_stop(&tim);
               
                    if ( set1 == set2 ) {     
                        //edge_color_main[i] = negid;
                        __sync_val_compare_and_swap(edge_color_main + i , 0, negid);   // edge_color_main[i] = id + 1;  ////// !!!!!! changed edge_color_helper to edge_color_main !!!!!!
                        negid--;
                        }
                    
                    else {
		            negid = -1;	
		            }
                    //i--;
                    timer_stop(&tim);
                } 
              /*  if ( edge_color_main[i] == MSF_EDGE ) {
                    break;
                }*/             
            //}

            /*else {
                if (edge_color_main[i] == id + 1 || edge_color_main[i] < 0) {
	    	        edge_color_main[i] = negid;
		    	    negid--;
		        }
		        else {
		            negid = -1;	
		        } */            
            //}

            i--;
        }
        pthread_barrier_wait(&bar);

    } // end of helper code 

    //printf("Loop counter %d for thread id = %d\n", loopcnt[thread_args->id], thread_args->id);
    pthread_exit(NULL);
}
