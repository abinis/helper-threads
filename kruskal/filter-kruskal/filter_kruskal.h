#ifndef FILTER_KRUSKAL_H
#define FILTER_KRUSKAL_H

#include "graph/edgelist.h"
#include "disjoint_sets/union_find_array.h"

#ifndef THRESHOLD
#define THRESHOLD 1024
#endif

// to use with filter_out below, for the concurrent version :)
extern union_find_node_t *fnode_array_global;

void filter_kruskal_init(edgelist_t *el,
                         union_find_node_t **fnode_array,
                         edge_t ***msf);

void filter_kruskal_concurrent_init(edgelist_t *el,
                                    union_find_node_t **fnode_array,
                                    edge_t ***msf);

void filter_kruskal(edgelist_t *el,
                    union_find_node_t *fnode_array,
                    edge_t **msf,
                    unsigned int *msf_edges);

void filter_kruskal_concurrent(edgelist_t *el,
                               union_find_node_t *fnode_array,
                               edge_t **msf,
                               unsigned int *msf_edges);

void filter_kruskal_rec(edgelist_t *el,
                        union_find_node_t *fnode_array,
                        int left,
                        int right,
                        edge_t **msf,
                        unsigned int *msf_edges);

void filter_kruskal_concurrent_rec(edgelist_t *el,
                                   /*union_find_node_t *fnode_array,*/
                                   int left,
                                   int right,
                                   edge_t **msf,
                                   unsigned int *msf_edges);

void custom_filter_kruskal(edgelist_t *el,
                           union_find_node_t *fnode_array,
                           int k,
                           edge_t **msf,
                           unsigned int *msf_edges);

void filter_kruskal_destroy(union_find_node_t *fnode_array,
                            edge_t **msf);

//int median_of_three(edge_t *edge_array, int left, int right);
//int median_of_rand_three(edge_t *edge_array, int left, int right);
//int median_of_rand_three_xor(edge_t *edge_array, int left, int right);

#endif
