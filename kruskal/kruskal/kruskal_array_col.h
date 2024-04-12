/**
 * @file
 * Kruskal-related functions declarations
 * uses union-find array implementation
 */ 

#ifndef KRUSKAL_COL_H_
#define KRUSKAL_COL_H_

#include "graph/edgelist_col.h"
#include "graph/adjlist.h"
#include "disjoint_sets/union_find_array.h"

void kruskal_init(edgelist_t *el,
                  adjlist_t *al,
                  union_find_node_t **fnode_array,
                  unsigned int **edge_membership);

void kruskal_sort_edges(edgelist_t *el);

void kruskal(edgelist_t *el, 
             union_find_node_t *node_array,  
             unsigned int *edge_membership);

void kruskal_destroy(adjlist_t *al,
                     union_find_node_t *fnode_array, 
                     unsigned int *edge_membership);
#endif

