/**
 * @file 
 * Adjacency list type definitions and function declarations
 */

#ifndef ADJLIST_H_
#define ADJLIST_H_

#include "graph.h"

/**
 * Graph vertex
 */ 
typedef struct node_st { 
    unsigned int id; //!< unique id
    weight_t weight; //!< vertex weight
    struct node_st *next; //!< next vertex in adjacency list
} node_t;

/**
 * Adjacency list graph representation
 */ 
typedef struct adjlist_st { 
    unsigned int nvertices; //!< number of vertices
    unsigned int nedges; //!< number of edges
    node_t **adj; //!< array of adjacency lists
    int is_undirected; //!< undirected flag
} adjlist_t;

/**
 * Keeps graph statistics
 */ 
typedef struct adjlist_stats_st{
    unsigned int nvertices;
    unsigned int nedges;
    unsigned int nparallel_edges; //!< number of parallel edges
    unsigned int nloops; //!< number of self edges
    float avg_outdegree; //!< average graph out-degree
    unsigned int max_outdegree; //!< maximum graph out-degree
    weight_t max_edge_weight; //!< maximum edge weight
    int is_undirected; //!< undirected flag
} adjlist_stats_t;


extern adjlist_t* adjlist_init(unsigned int nvertices);
extern void adjlist_destroy(adjlist_t *al);
extern int adjlist_insert_edge(adjlist_t *al, 
                                unsigned int s, 
                                unsigned int t, 
                                weight_t w, 
                                adjlist_stats_t *stats);
extern adjlist_t* adjlist_read(const char *filename, 
                               adjlist_stats_t *stats, 
                               int is_undirected);
extern void adjlist_print(adjlist_t *al);
extern void adjlist_analyze(adjlist_t *al, adjlist_stats_t *stats);
extern void adjlist_init_stats(adjlist_stats_t *stats);
extern void adjlist_print_stats(adjlist_stats_t *stats);

#endif
