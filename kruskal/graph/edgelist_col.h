/**
 * @file 
 * Edge list type definitions and function declarations
 */

#ifndef EDGELIST_COL_H_
#define EDGELIST_COL_H_

#include "adjlist.h"
#include "graph.h"

/**
 * Graph edge 
 */
typedef struct edge_st {
    unsigned int vertex1; //!< edge's first vertex
    unsigned int vertex2; //!< edge's second vertex
    weight_t weight; //!< edge weight
    char edge_color;
} edge_t;

/**
 * Edge list graph representation
 */ 
typedef struct edgelist_st {
    unsigned int nvertices; //!< number of vertices
    unsigned int nedges; //!< number of edges
    edge_t *edge_array; //!< array of edges
    int is_undirected; //!< undirected flag
} edgelist_t;

extern edgelist_t* edgelist_create(adjlist_t *al);
extern void edgelist_print(edgelist_t *el);
extern void edgelist_destroy(edgelist_t *el);
extern int edge_compare(const void *e1, const void *e2);

#endif
