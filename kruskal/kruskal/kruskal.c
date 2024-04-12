/**
 * @file 
 * Kruskal-related functions definitions
 */ 

#include "kruskal.h"

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
                  /*adjlist_t *al,*/
                  forest_node_t ***fnode_array,
                  unsigned int **edge_membership)
{
    unsigned int v, e;

    //assert(al);
    assert(el);
    assert(el->edge_array);

    // Create and initialize forest nodes
    *fnode_array = (forest_node_t**)malloc(el->nvertices * 
                                           sizeof(forest_node_t*) );
    if ( ! *fnode_array ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    // Make a set from each vertex of the graph
    for ( v = 0; v < /*al*/el->nvertices; v++ ) 
        (*fnode_array)[v] = make_set(v/*al->adj[v]*/);

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
    //unsigned int v, e;

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
 */ 
void kruskal(edgelist_t *el, 
             forest_node_t **fnode_array,
             unsigned int *edge_membership)
{
    unsigned int i;
    edge_t *pe;
    
    assert(fnode_array);
    assert(edge_membership);

    // For each edge, in order by non-decreasing weight...
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);

        forest_node_t *set1 = find_set(fnode_array[pe->vertex1]);
        forest_node_t *set2 = find_set(fnode_array[pe->vertex2]);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_sets(set1, set2);
            edge_membership[i] = 1;
        } 
    }
}

/**
 * Deallocate Kruskal structures
 * @param al pointer to adjacency list graph representation 
 * @param fnode_array forest nodes array
 * @param edge_membership designates whether an edge is part of the MSF
 */
void kruskal_destroy(/*adjlist_t *al,*/
                     edgelist_t *el,
                     forest_node_t **fnode_array, 
                     unsigned int *edge_membership)
{
    unsigned int v;

    //assert(al);
    
    for ( v = 0; v < el->nvertices; v++ )
       destroy_set(fnode_array[v]);
    free(fnode_array);
    free(edge_membership);
}
