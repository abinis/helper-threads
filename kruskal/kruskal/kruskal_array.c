/**
 * @file 
 * Kruskal-related functions definitions
 */ 

#include "kruskal_array.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <omp.h>

#include "machine/tsc_x86_64.h"
#include "util/util.h"
#include "heap/heap.h"
#include "filter-kruskal/filter_kruskal.h"
#include "dgal-quicksort/quicksort.h"
#include "cpp_functions/cpp_psort.h"

//extern FILE * cycles_f;
//extern FILE * jumps_f;

// declared as extern in filter_kruskal.h
// here's another declaration, probably redundant :P
//union_find_node_t *fnode_array_global;

static int is_sorted(edge_t * edge_array, int from, int n)
{
    int e;
    edge_t *e1 = &edge_array[from];
    for ( e = from+1; e < from+n; e++ ) {
        if ( edge_compare(e1, &edge_array[e]) > 0 )
            return 0;
        e1++;
    }

    return 1;
}

static inline void swap_edge(edge_t *e1, edge_t *e2)
{
    edge_t tmp;

    tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
}

/**
 * Allocate and initialize Kruskal structures
 * @param el pointer to sorted edge list 
 * @param al pointer to adjacency list graph representation 
 * @param fnode_array address of the pointer to forest nodes array
 * @param edge_membership address to the array that designates whether 
 *                        an edge is part of the MSF
 */
void kruskal_new_init(edgelist_t *el,
                      /*adjlist_t *al,*/
                      union_find_node_t **fnode_array,
                      edge_t ***msf)
                      //int **edge_membership)
{
    //unsigned int e;

    //assert(al);
    assert(el);
    assert(el->edge_array);

    // Create and initialize forest nodes
    *fnode_array = union_find_array_init(el->nvertices);

    // Create and initialize output array
    //*edge_membership = (int*)malloc(el->nedges * 
    //                                         sizeof(int));

    // at most nvertices-1 edges in the MSF :)
    *msf = (edge_t**)malloc((el->nvertices-1)*sizeof(edge_t*));
    if ( !msf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    //for ( e = 0; e < el->nedges; e++ ) 
    //    (*edge_membership)[e] = 0;
}

void kruskal_init(edgelist_t *el,
                      /*adjlist_t *al,*/
                      union_find_node_t **fnode_array,
                      int **edge_membership)
{
    unsigned int e;

    //assert(al);
    assert(el);
    assert(el->edge_array);

    // Create and initialize forest nodes
    *fnode_array = union_find_array_init(el->nvertices);

    // Create and initialize output array
    *edge_membership = (int*)malloc(el->nedges * 
                                             sizeof(int));

    if ( !edge_membership ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    for ( e = 0; e < el->nedges; e++ ) 
        (*edge_membership)[e] = 0;
}

void kruskal_init_shrt(edgelist_t *el,
                  /*adjlist_t *al,*/
                  union_find_node_t **fnode_array,
                  short int **edge_membership)
{
    unsigned int e;

    //assert(al);
    assert(el);
    assert(el->edge_array);

    // Create and initialize forest nodes
    *fnode_array = union_find_array_init(el->nvertices);

    // Create and initialize output array
    *edge_membership = (short int*)malloc(el->nedges * 
                                             sizeof(short int));
    if ( ! *edge_membership ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    for ( e = 0; e < el->nedges; e++ ) 
        (*edge_membership)[e] = 0;
}

void kruskal_init_char(edgelist_t *el,
                  /*adjlist_t *al,*/
                  union_find_node_t **fnode_array,
                  char **edge_membership)
{
    unsigned int e;

    //assert(al);
    assert(el);
    assert(el->edge_array);

    // Create and initialize forest nodes
    *fnode_array = union_find_array_init(el->nvertices);

    // Create and initialize output array
    *edge_membership = (char*)malloc(el->nedges * 
                                             sizeof(char));
    if ( ! *edge_membership ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    for ( e = 0; e < el->nedges; e++ ) 
        (*edge_membership)[e] = 0;
}

/**
 * Sort edge list
 * @param el pointer to sorted edge list 
 */
void kruskal_sort_edges(edgelist_t *el, int nthreads)
{
    assert(el);
    assert(el->edge_array);

    // Sort edge list by non-decreasing weight
#ifdef CPP_SORT
    cpp_sort_edge_arr(el->edge_array, el->nedges, nthreads);
#else
    qsort(el->edge_array, el->nedges, sizeof(edge_t), edge_compare);
#endif
}

void kruskal_sort_edges_w_id(edgelist_w_id_t *el)
{
    assert(el);
    assert(el->edge_array);

    // Sort edge list by non-decreasing weight
    qsort(el->edge_array, el->nedges, sizeof(edge_w_id_t), edge_w_id_compare);
}

/**
 * Runs Kruskal MSF algorithm
 * @param el pointer to sorted edge list 
 * @param fnode_array pointer to forest nodes array
 * @param edge_membership designates whether an edge is part of the MSF
 *     1 if it is
 *     2 if it would form a cycle
 *     0 if it wasn't examined at all; MSF completed before it was reached
 */ 
void kruskal_new(edgelist_t *el, 
                 union_find_node_t *array,
                 edge_t **msf,
                 unsigned int *msf_edges)
                 //int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    //assert(array);
    //assert(edge_membership);

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            msf[*msf_edges] = pe;
            (*msf_edges)++;
            //edge_membership[i] = 1;
        } /*else
            edge_membership[i] = 2;*/
    }
}

void kruskal(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else
            edge_membership[i] = 2;
    }
}

void kruskal_w_id(edgelist_w_id_t *el, 
                  union_find_node_t *array,
                  int *edge_membership)
{
    unsigned int i;
    edge_w_id_t *pe;
    
    assert(array);
    assert(edge_membership);

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else
            edge_membership[i] = 2;
    }
}

unsigned int kruskal_up_to(edgelist_t *el,
                   union_find_node_t *array,
                   int *edge_membership,
                   unsigned int from,
                   unsigned int upto)
{
    unsigned int i, ret = 0;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    // For each edge until 'upto', in order by non-decreasing weight...
    int set1, set2;
    for ( i = from; i < upto; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else
            edge_membership[i] = 2;
    }
    // ...only mark cycles for the rest!
    while ( i < el->nedges ) {
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to same forest
        if ( set1 == set2 ) {
            edge_membership[i] = 2;
            ret++;
        }

        i++;
    }

    return ret;
}

void kruskal_up_to_prepare(edgelist_t *el,
                           int *edge_membership,
                           unsigned int upto)
{
    unsigned int i;

    for (i = upto; i < el->nvertices; i++)
        edge_membership[i] = 0;
    
}

void kruskal_shrt(edgelist_t *el, 
             union_find_node_t *array,
             short int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else
            edge_membership[i] = 2;
    }
}

void kruskal_char(edgelist_t *el, 
             union_find_node_t *array,
             char *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else
            edge_membership[i] = 2;
    }
}

unsigned int kruskal_get_msf_edge_count_char(edgelist_t *el, char *edge_membership)
{
    assert(el);
    assert(edge_membership);

    unsigned int msf_edge_count = 0;
    long int e;

    for ( e = 0; e < el->nedges; e++ )
        msf_edge_count += ( edge_membership[e] == 1 );

    return msf_edge_count;
}
    
weight_t kruskal_get_msf_weight_char(edgelist_t *el, char *edge_membership)
{
    assert(el);
    assert(edge_membership);

    weight_t msf_weight = 0.0;
    long int e;

    for ( e = 0; e < el->nedges; e++ )
        if ( edge_membership[e] == 1 )
            msf_weight += el->edge_array[e].weight;

    return msf_weight;
}

/**
 * Same as above, but now:
 * @param edge_membership is already filled by a previous run
 * This algorithm simulates the main thread of mt_kruskal,
 * where ALL the cycle forming edges have been marked by the
 * helper threads :)
 *
 * return value: the number of edges skipped
 */ 
unsigned int kruskal_oracle(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int cycles_skipped = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 

        if ( edge_membership[i] != 0 ) {
            //fprintf(cycles_f, "%d\n", i);
            cycles_skipped++;
            continue;
        }
    
        pe = &(el->edge_array[i]);

        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }

    return cycles_skipped;
}

unsigned int kruskal_oracle_func(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int cycles_skipped = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 

        if ( edge_membership[i] != 0 ) {
            //fprintf(cycles_f, "%d\n", i);
            cycles_skipped++;
            continue;
        }
    
        pe = &(el->edge_array[i]);

        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }

    return cycles_skipped;
}

unsigned int kruskal_oracle_shrt(edgelist_t *el, 
             union_find_node_t *array,
             short int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int cycles_skipped = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 

        if ( edge_membership[i] != 0 ) {
            //fprintf(cycles_f, "%d\n", i);
            cycles_skipped++;
            continue;
        }
    
        pe = &(el->edge_array[i]);

        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }

    return cycles_skipped;
}

unsigned int kruskal_oracle_char(edgelist_t *el, 
             union_find_node_t *array,
             char *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int cycles_skipped = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 

        if ( edge_membership[i] != 0 ) {
            //fprintf(cycles_f, "%d\n", i);
            cycles_skipped++;
            continue;
        }
    
        pe = &(el->edge_array[i]);

        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }

    return cycles_skipped;
}

/**
 * This time, 
 * @param edge_membership is already filled by a previous run
 * This algorithm simulates the main thread of mt_kruskal,
 * where ALL the cycle forming edges have been marked by the
 * helper threads, AND the main thread jumps over them :)
 *
 * return value: the number of edges skipped
 */ 
unsigned int kruskal_oracle_jump(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int jumps = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    unsigned int next = 1;
    for ( i = 0; i < el->nedges; i+=next ) { 

        if ( edge_membership[i] < 0 ) {
            next = edge_membership[i]*(-1);
            //fprintf(jumps_f, "%d -> %d\n", i, i+next);
            //i += (next-1);
            jumps += next;
            continue;
        }

        next = 1;

        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }

    return jumps;
}

unsigned int kruskal_oracle_jump_shrt(edgelist_t *el, 
             union_find_node_t *array,
             short int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int jumps = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 

        if ( edge_membership[i] < 0 ) {
            unsigned int next = edge_membership[i]*(-1);
            //fprintf(jumps_f, "%d -> %d\n", i, i+next);
            i += (next-1);
            jumps += next;
            continue;
        }

        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }

    return jumps;
}

unsigned int kruskal_oracle_jump_char(edgelist_t *el, 
             union_find_node_t *array,
             char *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);

    unsigned int jumps = 0;
    unsigned int hops = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    for ( i = 0; i < el->nedges; i++ ) { 

        if ( edge_membership[i] < 0 ) {
            unsigned int next = edge_membership[i]*(-1);
            //fprintf(jumps_f, "%d -> %d\n", i, i+next);
            i += (next-1);
            jumps += next;
            hops++;
            continue;
        }

        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // should never be reached
            edge_membership[i] = 2;
            //assert(0);
        }
    }
    printf("hops=%u\n", hops);

    return jumps;
}

/**
 * Prepares an edge_membership array, already filled by a 
 * previous run, by leaving only 0s and 2s (edges marked as cycles) :)
 */
void kruskal_oracle_prepare(edgelist_t *el,
                            union_find_node_t *array, 
                            int *edge_membership)
{
    int i;
    int mask = 1 << 1;
    for ( i = 0; i < el->nedges; i++ )
        edge_membership[i] &= mask;
    
    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

void kruskal_oracle_prepare_shrt(edgelist_t *el,
                            union_find_node_t *array, 
                            short int *edge_membership)
{
    int i;
    short int mask = 1 << 1;
    for ( i = 0; i < el->nedges; i++ )
        edge_membership[i] &= mask;
    
    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

void kruskal_oracle_prepare_char(edgelist_t *el,
                            union_find_node_t *array, 
                            char *edge_membership)
{
    int i;
    char mask = 1 << 1;
    for ( i = 0; i < el->nedges; i++ )
        edge_membership[i] &= mask;
    
    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

/**
 * Alternate version of the above function, uses if statement to
 * determine which elements to zero-out -- instead of simply applying
 * a bitmask to all the elements of the array ;)
 * Turns out, this version is slower :P
 */
void kruskal_oracle_prepare_alt(edgelist_t *el,
                                union_find_node_t *array, 
                                int *edge_membership) 
{
    int i;
    for ( i = 0; i < el->nedges; i++ ) {
        if ( edge_membership[i] == 1 )
            edge_membership[i] = 0;
    }
    
    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

/**
 * The edges to be checked (and joined, since they are guaranteed to belong
 * to the MSF) are placed consecutively at the front part of the 
 * @param edge_array. The remaining positions are filled with a single
 * 'jump streak' :) --> essentially, the edge_array (and the corresponding
 * edge_membership array) is "compressed", exhibiting memory reference
 * locality :)
 *
 * In the case below, the 'main thread' knows this, and thus checks only
 * the first 'end' edges. 
 */
void kruskal_oracle_comp(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership, edge_t *edge_array, int end)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);
    assert(edge_array);

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    // check (and join) the first end edges :)
    for ( i = 0; i < end; i++ ) { 
        pe = &(edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else // should never be reached
            edge_membership[i] = 2;
    }
}

/* TODO: Prepare the edge_array and the accompanying edge_membership array
         exactly like multiple oracle helper threads working on it would do,
         e.g. for 2 helper threads:
         mt         ht#1                    ht#2
         |  |  |... | 0| 0| 0| 0|-4|-3|-2|-1| 0| 0| 0| 0| 0|-3|-2|-1|

         Well, since our oracle simulations assume perfect knowledge of
         every edge's status, the first (mt) part could be omitted (as if
         the hts work on the entire input ("overlap").
*/
int kruskal_oracle_prepare_comp(edgelist_t *el,
                                  union_find_node_t *array,
                                  int *edge_membership,
                                  edge_t *edge_array)
{
    int i, end = 0;
    for ( i = 0; i < el->nedges; i++ ) {
        if ( edge_membership[i] == 1 ) {
            edge_membership[end] = 0;
            // also bring the actual edges together!
            edge_array[end++] = edge_array[i];
        }
    }
    // then, fill the last nedges-end positions with jumps :)
    // *all the small jumps should be pushed together into a
    // single jump streak at the end of the array, e.g.
    // |-1| 0| 0|-2|-1| 0| 0| 0|-1| --> | 0| 0| 0| 0| 0|-4|-3|-2|-1|
    i = end;
    while ( i < el->nedges )
    {
        edge_membership[i] = i - el->nedges;
        i++;
    }
   
    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }

    return end;
}

unsigned int kruskal_oracle_comp_mt(edgelist_t *el, 
                                    union_find_node_t *array,
                                    int *edge_membership,
                                    edge_t *edge_array)
{
    unsigned int i;
    edge_t *pe;
    
    assert(array);
    assert(edge_membership);
    assert(edge_array);

    unsigned int jumps = 0;

    // For each edge, in order by non-decreasing weight...
    int set1, set2;
    unsigned int next = 1;
    for ( i = 0; i < el->nedges; i+=next ) { 

        if ( edge_membership[i] < 0 ) {
            next = edge_membership[i]*(-1);
            //fprintf(jumps_f, "%d -> %d\n", i, i+next);
            //i += (next-1);
            jumps += next;
            continue;
        }

        next = 1;

        pe = &(edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            edge_membership[i] = 1;
        } else {
            // this can be reached now, since the main thread, simulated here,
            // checks the first part of the edges by itself :)
            edge_membership[i] = 2;
            // we also update the 'jumps' counter :)
            jumps++;
            //assert(0);
        }
    }

    return jumps;
}

/**
 * That's the TODO outlined above :P
 * In the 'real' version, the helper threads bring close together
 * the edges marked with 0, instead of 1 (those that they are not
 * able to decide upon)
 */ 
void kruskal_oracle_prepare_comp_mt(edgelist_t *el,
                                  union_find_node_t *array,
                                  int *edge_membership,
                                  edge_t *edge_array,
                                  int begin, int end, int nthr)
{
    int t, from, to, i, e /*= 0*/;

    int chunk_size = ( end - begin ) / nthr;
    int rem = ( end - begin ) % nthr;
    // for each of the nthr threads...
    for ( t = 0; t < nthr; t++ ) {
        // ...compute bounds (each thread gets a chunk_size sized chunk
        // to work on, +1 whenever remainder distribution applies)
        from = begin + ( (t < rem) ? t*(chunk_size+1) : rem*(chunk_size+1)+(t-rem)*chunk_size );
        to = from + ( (t < rem) ? chunk_size+1 : chunk_size );
        e = from;
        for ( i = from; i < to; i++ ) {
            if ( edge_membership[i] == 1 ) {
                edge_membership[e] = 0;
                // also bring the actual edges together!
                // Edit! sort them using insertion sort!
                // well, in the version below actually :P
                edge_array[e++] = edge_array[i];
                /*
                edge_t key = edge_array[i];
                int j = e-1;
                while ( j >= from && edge_array[j].weight > key.weight ) {
                    edge_array[j+1] = edge_array[j];
                    j--;
                }
                edge_array[j+1] = key;
                e++;
                */
            }
        }
        // then, fill the last to-e positions with jumps :)
        // *all the small jumps should be pushed together into a
        // single jump streak at the end of the array, e.g.
        // |-1| 0| 0|-2|-1| 0| 0| 0|-1| --> | 0| 0| 0| 0| 0|-4|-3|-2|-1|
        //printf("%d\n", CHAR_MIN);
        i = e;
        while ( i < to )
        {
            edge_membership[i] = (i - to) % CHAR_MIN;
            i++;
        }
    }
   
    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }

    //return e;
}

void kruskal_ht_scheme_simulation(edgelist_t *unsorted_el,
                                  union_find_node_t *array,
                                  //int *edge_membership,
                                  int k, 
                                  /*unsigned int nedges,*/
                                  int main_threads,
                                  int helper_threads,
                                  edge_t **result,
                                  unsigned int *msf_edges)
{
    int i;

    edge_t *pe;
    int set1, set2;
    
    assert(array);
    //assert(edge_membership);

    // for convenience
    edge_t *edge_array = unsorted_el->edge_array;
    *msf_edges = 0;

    //union_find_t *box = union_find_array_header_create(array,unsorted_el->nvertices);
    //union_find_array_print(box);

    //printf("mt    [%d, %d)\n", 0, k);

    // Main Thread code

#ifdef PROFILE
    tsctimer_t tim;
    double hz;
#endif

#ifdef USE_FILTER_KRUSKAL
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

#ifdef CONCURRENT_MAIN
    if ( main_threads > 1 ) {
        omp_set_num_threads(main_threads);
        //filter_kruskal_concurrent_init(unsorted_el,array,msf);
        fnode_array_global = array;
        filter_kruskal_concurrent_rec(unsorted_el,/*array,*/0,k,result,msf_edges);
    } else 
        filter_kruskal_rec(unsorted_el,array,0,k,result,msf_edges);
    //union_find_array_print(box);
#else
    filter_kruskal_rec(unsorted_el,array,0,k,result,msf_edges);
#endif

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - main filter-k    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#else
//#ifdef USE_CLASSIC_KRUSKAL
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    // First, sort the leftmost part :)
#ifdef CONCURRENT_MAIN
    cpp_sort_edge_arr(edge_array, k, main_threads);
#else
    qsort(/*unsorted_el->*/edge_array, k, sizeof(edge_t), edge_compare);
#endif

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - main sort       cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    //for ( i = 0; i < unsorted_el->nedges; i++ )
    //    printf("(%d %d %f)\n", edge_array[i].vertex1,
    //                           edge_array[i].vertex2, 
    //                           edge_array[i].weight);

    //printf("%s - qsort ok!\n", __FUNCTION__);

    // For the first edges, upto k...

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

    for ( i = 0; i < k; i++ ) { 
        pe = &(/*unsorted_el->*/edge_array[i]);
    
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            //edge_membership[i] = 1;
            result[*msf_edges] = pe;
            (*msf_edges)++;
        } /*else
            edge_membership[i] = 2;*/
    }

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - main thread     cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#endif
    //printf("%s - main thread first part ok :)\n", __FUNCTION__);
    //weight_t msf_weight = 0.0;
    //for ( i = 0; i < *msf_edges; i++ ) {
    //    msf_weight += result[i]->weight;
    //    edge_print((edge_t*)result[i]);
    //}
    //printf("msf edges found: %u, weight: %f\n", *msf_edges, msf_weight);

    // Helper Thread code
#ifdef CONCURRENT_FILTER
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    omp_set_num_threads(helper_threads);
    //union_find_array_print(box);
    fnode_array_global = array;
    long end;
    #pragma omp parallel
    {
        end = quicksort_filter_concurrent_inplace(edge_array, k, unsorted_el->nedges-1);
    }
    printf("concurrent filtering, edges left = %ld\n", end-k);
    //union_find_array_print(box);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - concurrent filt  cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
    //union_find_array_print(box);
#endif
#else
    int t;
    // we 'll need the starting (from) and ending (to) positions for each
    // helper thread, as well as the # of edges (e) that survived filtering,
    // and are now compressed and sorted at the beginning of each chunk :)
    int *from, *to, *e;
    from = malloc_safe(helper_threads*sizeof(int));
    to = malloc_safe(helper_threads*sizeof(int));
    e = malloc_safe(helper_threads*sizeof(int));

    int length = unsorted_el->nedges - k;

    int chunk_size = ( length ) / helper_threads; 
    int rem = ( length ) % helper_threads;
    //printf("len = %d, chunk_size = %d, rem = %d\n", length, chunk_size, rem);
    // for each of the nthr threads...
    for ( t = 0; t < helper_threads; t++ ) {

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
        long cycles_found = 0;
        // ...compute bounds (each thread gets a chunk_size sized chunk
        // to work on, +1 whenever remainder distribution applies)
        from[t] = k + ( (t < rem) ? t*(chunk_size+1) : rem*(chunk_size+1)+(t-rem)*chunk_size );
        to[t] = from[t] + ( (t < rem) ? chunk_size+1 : chunk_size );
        //printf("ht#%d [%d, %d)\n", t, from[t], to[t]);
        // first, perform a backward pass, where cycle-forming edges
        // are detected :)
        // We will need an edge_membership array to mark these edges,
        // so we can ignore them in the next (forward) pass ;)
#ifndef MARK_AND_PUSH
        char *edge_membership = calloc(to[t]-from[t], sizeof(char));
#else
        int cyc_pos = to[t]-1;
#endif

#ifndef MARK_AND_PUSH
        for ( i = to[t]-1; i >= from[t]; i-- ) {
#else
        for ( i = cyc_pos; i >= from[t]; i-- ) {
#endif

/*MAYBE? A: makes sense only for multi-threaded :)
#ifndef MARK_AND_PUSH
            if ( edge_membership[i-from[t]] == 1 )
                continue;
#endif
*/
            pe = &(/*unsorted_el->*/edge_array[i]);

            // reminder: hts don't perform path compression :)
            set1 = union_find_array_find_helper(array, pe->vertex1);
            set2 = union_find_array_find_helper(array, pe->vertex2);

            // cycle found, mark the edge! -- or push it ;)
            if ( set1 == set2 ) {
/*TODO*/
#ifndef MARK_AND_PUSH
                edge_membership[i-from[t]] = 1;
#else
                //swap_edge(&edge_array[cyc_pos--],pe); 
                edge_array[i] = edge_array[cyc_pos--];
#endif

                cycles_found++;
            }
        
        }
        printf("tid#%d cycles_found:%ld\n", t, cycles_found);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
#ifndef MARK_AND_PUSH
    fprintf(stdout, "sim - ht#%2d bwd/pass   cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    t,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#else
    fprintf(stdout, "sim - ht#%2d mark&push  cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    t,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#endif

        //printf("%s - helper threads cycle detect ok :)\n", __FUNCTION__);

        // second, perform a forward pass, compressing and sorting 
        // the surviving edges! :)

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

#ifndef MARK_AND_PUSH
        e[t] = from[t];

        for ( i = from[t]; i < to[t]; i++ ) {
            if ( edge_membership[i-from[t]] == 0 ) {
#ifdef USE_QSORT
                edge_array[e[t]++] = edge_array[i];

#elif defined USE_ISORT                
                edge_t key = edge_array[i];
                int j = e[t]-1;
                while ( j >= from[t] && edge_array[j].weight > key.weight ) {
                    edge_array[j+1] = edge_array[j];
                    j--;
                }
                edge_array[j+1] = key;
                e[t]++;
#endif
            }
        }

        // finally, add the guard element :)
        // (we can't use -1 due to unsigned int)
        // --> actually, we *multiply* the weight of the
        // last element by -1; we don't add anything ;)
        // hmm maybe not :P
        //edge_array[from[t]+e[t]].vertex1 = 0;
        //edge_array[from[t]+e[t]].vertex2 = 0;
        //edge_array[from[t]+e[t]].weight = -1.0;
        //if ( e[t] > from[t] )
        //    edge_array[e[t]-1].weight *= -1.0;

        e[t] = e[t] - from[t];

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - ht#%2d fwd/pass   cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    t,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#else
        e[t] = cyc_pos + 1 - from[t];
#endif

        assert( e[t] == to[t]-from[t]-cycles_found );

        //printf("ht#%d %d\n", t, e[t]);

        //for ( i = from[t]; i < from[t]+e[t]; i++ )
        //    printf("(%d %d %f)\n", edge_array[i].vertex1,
        //                           edge_array[i].vertex2, 
        //                           edge_array[i].weight);
#ifdef USE_QSORT
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
#ifdef CPP_SORT
        cpp_sort_edge_arr(edge_array+from[t], e[t], 1);
#else
        qsort(edge_array+from[t], e[t], sizeof(edge_t), edge_compare);
#endif
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - ht#%2d +qsort     cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    t,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#endif

        //printf("ht#%d sorted\n", t);

        //assert( is_sorted(edge_array,from[t],e[t]) );
        //edge_t * edgep = &(edge_array[from[t]]);
        //while ( edgep->weight > 0 ) {
        //    printf("(%d %d %f)\n", edgep->vertex1,
        //                           edgep->vertex2, 
        //                           edgep->weight);
        //    edgep += 1;
        //}
        
        //for ( i = from[t]; i < from[t]+e[t]; i++ )
        //    printf("(%d %d %f)\n", edge_array[i].vertex1,
        //                           edge_array[i].vertex2, 
        //                           edge_array[i].weight);

        //for ( i = from[t]; i <= from[t]+e[t]; i++ )
        //    printf("(%d %d %f)\n", edge_array[i].vertex1,
        //                           edge_array[i].vertex2, 
        //                           edge_array[i].weight);
        // then, fill the last to-e positions with jumps :)
        // *all the small jumps should be pushed together into a
        // single jump streak at the end of the array, e.g.
        // |-1| 0| 0|-2|-1| 0| 0| 0|-1| --> | 0| 0| 0| 0| 0|-4|-3|-2|-1|
        // i = e;
        // while ( i < to )
        // {
        //     edge_membership[i] = i - to;
        //     i++;
        // }

#ifndef MARK_AND_PUSH
        free(edge_membership);
#endif
    }
#endif

    // Main Thread again: check/decide upon the remaining edges, in order
    // of ascending weight. We achieve this by repeatedly picking the
    // minimum-weight edge among the nthr sorted arrays produced by the hts!
    // Two (2) options here:
    //   1 - find min every time
    //   2 - use a min-heap 
    // On separate experiments, we found that a min-heap is considerably
    // faster! :)
    //int *pos;
    //pos = malloc_safe(nthr*sizeof(int));

    //printf("%s - main thread again!\n", __FUNCTION__ );

#ifndef CONCURRENT_FILTER
    long edges_left = 0;
    for ( t = 0; t < helper_threads; t++ ) {
        edges_left += e[t];
    }
    printf("  standard filtering, edges left = %ld\n", edges_left);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    heap_aug_t *hp = heap_aug_create_empty(helper_threads, edge_compare);

    for ( t = 0; t < helper_threads; t++ ) {
        if ( e[t] > 0 )
            heap_aug_add(hp, &(/*unsorted_el->*/edge_array[from[t]]),
                            &(edge_array[from[t]+e[t]-1]));
    }

    heap_aug_construct(hp);

    //printf("### init hs = %lu\n", hp->hs);
    //heap_aug_print(hp, edge_print);
    //printf("###\n");

    //unsigned int touched = 0;
    while ( !heap_aug_is_empty(hp) ) {

        edge_t *min = (edge_t*)heap_aug_peek_min(hp);

        //printf("### del(%d %d %f) hs = %lu\n", min->vertex1, min->vertex2, min->weight, hp->hs);
        //heap_aug_print(hp, edge_print);
        //printf("###\n");
        //printf("%f\n", min->weight);

        //touched++;

        set1 = union_find_array_find(array, min->vertex1);
        set2 = union_find_array_find(array, min->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            // find index within edge_membership by doing some
            // pointer arithmetic :)
            // --> doesn't matter, unless we 'parallelise' it with
            // edge_array after all that partitioning/compressing/sorting :)
            //edge_membership[min-edge_array] = 1;
            result[*msf_edges] = min;
            (*msf_edges)++;
        } /*else
            edge_membership[min-edge_array] = 2;*/


        //if ( min->weight < 0 ) {
        //    // last element, just revert :)
        //    min->weight *= -1.0;
        //} else {
            edge_t *next = (edge_t*)(min+1);
            heap_aug_increase_root_key(hp, next);

            //printf("### ins(%d %d %f) hs = %lu\n", next->vertex1, next->vertex2, next->weight, hp->hs);
            //heap_aug_print(hp, edge_print);
            //printf("###\n");
        //}
    }

    //unsigned int total = 0;
    //for ( t = 0; t < nthr; t++ )
    //    total += e[t];

    //printf("total = %u, touched = %u\n", total, touched);
    //assert( touched == total );

    heap_aug_destroy(hp);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - main thr heap    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    free(from);
    free(to);
    free(e);

#else
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    // if we do concurrent filtering, we end up with a single block;
    // sort it first!...
//#ifdef CPP_SORT
    cpp_sort_edge_arr(edge_array+k, end-k, helper_threads);
//#else
    //qsort(edge_array+k, end-k, sizeof(edge_t), edge_compare);
//#endif
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - sort filtered    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    // ...then check the remaining edges :)
    //union_find_array_print(box);
    for ( i = k; i < end; i++ ) {
        pe = &(edge_array[i]);
        set1 = union_find_array_find(array, pe->vertex1);
        set2 = union_find_array_find(array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(array, set1, set2);
            result[*msf_edges] = pe;
            (*msf_edges)++;
        } 
    }
    //union_find_array_header_destroy(box);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sim - main thr check   cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
#endif
    //union_find_array_header_destroy(box);

}

/**
 * Prepares an edge_membership array, already filled by a 
 * previous run, by leaving 0s intact, zeroing 1s, and replacing
 * 2s with runs of negative numbers, up to INT_MIN -- well, 
 * this isn't tested actually, and maybe it should (TODO), in order to
 * avoid overflows;
 * the edge_membership array is read backwards of course :)
 */
void kruskal_oracle_jump_prepare(edgelist_t *el,
                                 union_find_node_t *array, 
                                 int *edge_membership)
{
    int i;
    int negid = -1;
    for ( i = el->nedges-1; i >= 0; i-- ) {

        // if edge isn't marked as a cycle, skip it...
        if ( !(edge_membership[i] < 0) && (edge_membership[i] != 2) ) {
            // ...this also ends our streak
            edge_membership[i] = 0;
            negid = -1;
            continue;
        }

        edge_membership[i] = negid;
        negid--;

    }

    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

/**
 * Same as above, only this time 2s are replaced 
 * with runs of negative numbers, up to -128 (CHAR_MIN), 
 * in order to test a smaller size edge_membership array,
 * namely a char array instead of an int one
 */
void kruskal_oracle_jump_prepare_char(edgelist_t *el,
                                 union_find_node_t *array, 
                                 char *edge_membership)
{
    int i;
    int negid = -1;
    int max = 1 << (sizeof(char)*8 - 1);
    for ( i = el->nedges-1; i >= 0; i-- ) {

        //negid = ( negid % (max*(-1)) );
        //if ( negid == 0 ) negid = (-1)*max;
        if ( negid < max*(-1) )
            negid = -1;

        // if edge isn't marked as a cycle, skip it...
        if ( !(edge_membership[i] < 0) && (edge_membership[i] != 2) ) {
            // ...this also ends our streak
            edge_membership[i] = 0;
            negid = -1;
            continue;
        }

        edge_membership[i] = negid;
        negid--;

    }

    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

/**
 * Same as above, only this time 2s are replaced 
 * with runs of negative numbers, up to SHRT_MIN, 
 * in order to test a smaller size edge_membership array,
 * namely a char array instead of an int one
 */
void kruskal_oracle_jump_prepare_shrt(edgelist_t *el,
                                 union_find_node_t *array, 
                                 short int *edge_membership)
{
    int i;
    int negid = -1;
    int max = 1 << (sizeof(short int)*8 - 1);
    for ( i = el->nedges-1; i >= 0; i-- ) {

        //negid = ( negid % (max*(-1)) );
        //if ( negid == 0 ) negid = (-1)*max;
        if ( negid < max*(-1) )
            negid = -1;

        // if edge isn't marked as a cycle, skip it...
        if ( !(edge_membership[i] < 0) && (edge_membership[i] != 2) ) {
            // ...this also ends our streak
            edge_membership[i] = 0;
            negid = -1;
            continue;
        }

        edge_membership[i] = negid;
        negid--;

    }

    // also reset our union-find structure ;)
    for (i = 0; i < el->nvertices; i++) {
        array[i].parent = i;
        array[i].rank = 0;
    }
}

/**
 * Deallocate Kruskal structures
 * @param al pointer to adjacency list graph representation 
 * @param fnode_array forest nodes array
 * @param edge_membership designates whether an edge is part of the MSF
 */
void kruskal_new_destroy(/*adjlist_t *al,i*/
                     union_find_node_t *fnode_array, 
                     edge_t **msf)
                     //void *edge_membership)
{
    //assert(al);
    assert(fnode_array);
    assert(msf);
    
    union_find_array_destroy(fnode_array);
    //free(edge_membership);
    free(msf);
}

void kruskal_destroy(/*adjlist_t *al,i*/
                     union_find_node_t *fnode_array, 
                     void *edge_membership)
{
    //assert(al);
    assert(fnode_array);
    assert(edge_membership);
    
    union_find_array_destroy(fnode_array);
    free(edge_membership);
}
