/**
 * @file 
 * Edge list type definitions and function declarations
 */

#ifndef EDGELIST_H_
#define EDGELIST_H_

#include "adjlist.h"
#include "graph.h"

/**
 * Graph edge 
 */
typedef struct edge_st {
    unsigned int vertex1; //!< edge's first vertex
    unsigned int vertex2; //!< edge's second vertex
    weight_t weight; //!< edge weight
} edge_t;

// graph edge with added id field :)
typedef struct edge_st_id {
    unsigned int vertex1; //!< edge's first vertex
    unsigned int vertex2; //!< edge's second vertex
    weight_t weight; //!< edge weight
    unsigned int id; //!< edge id (actually, original position)
} edge_w_id_t;

void edge_print(void /*(edge_t*/ *edge);
void edge_w_id_print(void /*edge_w_id_t*/ *edge);

/**
 * Edge list graph representation
 */ 
typedef struct edgelist_st {
    unsigned int nvertices; //!< number of vertices
    unsigned int nedges; //!< number of edges
    edge_t *edge_array; //!< array of edges
    int is_undirected; //!< undirected flag
} edgelist_t;

// again, version with id
typedef struct edgelist_st_id {
    unsigned int nvertices; //!< number of vertices
    unsigned int nedges; //!< number of edges
    edge_w_id_t *edge_array; //!< array of id-augmented (:P) edges
    int is_undirected; //!< undirected flag
} edgelist_w_id_t;

extern edgelist_t* edgelist_create(adjlist_t *al);
extern edgelist_t* edgelist_read(const char *filename,
                                 int is_undirected,
                                 int is_vertex_numbering_zero_based);

// this one reads the input file into edge_w_id_t structs
extern edgelist_w_id_t* edgelist_w_id_read(const char *filename,
                                      int is_undirected,
                                      int is_vertex_mumbering_zero_based);

extern void edgelist_print(edgelist_t *el);
extern void edgelist_w_id_print(edgelist_w_id_t *el);
edgelist_t* edgelist_single_read(const char *filename,
                                        int is_undirected,
                                        int is_vertex_numbering_zero_based);
int edgelist_dump(edgelist_t *el, const char *outfile);
edgelist_t * edgelist_load(const char *dumpfile);
edgelist_t * edgelist_choose_input_method(const char *filename,
                                          int is_undirected,
                                          int is_vertex_numbering_zero_based,
                                          char **method_chosen);
extern void edgelist_destroy(edgelist_t *el);
extern int edge_compare(const void *e1, const void *e2);
int edge_w_id_compare(const void *e1, const void *e2);
int edge_equality_check(edge_t *e1, edge_t *e2);

#endif
