/**
 * @file
 * Kruskal-related functions declarations
 */ 

#ifndef KRUSKAL_H_
#define KRUSKAL_H_

#include "graph/edgelist.h"
#include "graph/adjlist.h"
#include "disjoint_sets/union_find.h"

void kruskal_init(edgelist_t *el,
                  /*adjlist_t *al,*/
                  forest_node_t ***fnode_array,
                  unsigned int **edge_membership);

void kruskal_sort_edges(edgelist_t *el);

void kruskal(edgelist_t *el, 
             forest_node_t **node_array,  
             unsigned int *edge_membership);

void kruskal_destroy(/*adjlist_t *al,*/
                     edgelist_t *el,
                     forest_node_t **fnode_array, 
                     unsigned int *edge_membership);
#endif
