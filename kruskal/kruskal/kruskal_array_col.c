/**
 * @file 
 * Kruskal-related functions definitions
 */ 

#include "kruskal_array_col.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * Allocate and initialize Kruskal structures
 * @param el pointer to sorted edge list 
 * @param al pointer to adjacency list graph representation 
 * @param fnode_array address of the pointer to forest nodes array
 * @param edge_membership address to the array that designates whether 
 *                        an edge is part of the MSF
 */
void kruskal_init(edgelist_t *el,
                  adjlist_t *al,
                  union_find_node_t **fnode_array,
                  unsigned int **edge_membership)
{
    unsigned int e;

    assert(al);
    assert(el);
    assert(el->edge_array);

    // Create and initialize forest nodes
    *fnode_array = union_find_array_init(el->nvertices);

    // Create and initialize output array
    *edge_membership = (unsigned int*)malloc(el->nedges * 
                                             sizeof(unsigned int));
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
void kruskal_sort_edges(edgelist_t *el)
{
    assert(el);
    assert(el->edge_array);

    // Sort edge list by non-decreasing weight
    qsort(el->edge_array, el->nedges, sizeof(edge_t), edge_compare);
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
void kruskal(edgelist_t *el, 
             union_find_node_t *array,
             unsigned int *edge_membership)
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

/**
 * Deallocate Kruskal structures
 * @param al pointer to adjacency list graph representation 
 * @param fnode_array forest nodes array
 * @param edge_membership designates whether an edge is part of the MSF
 */
void kruskal_destroy(adjlist_t *al,
                     union_find_node_t *fnode_array, 
                     unsigned int *edge_membership)
{
    assert(al);
    
    union_find_array_destroy(fnode_array);
    free(edge_membership);
}

