/**
 * @file
 * Kruskal-related functions declarations
 * uses union-find array implementation
 */ 

#ifndef KRUSKAL_ARRAY_H_
#define KRUSKAL_ARRAY_H_

#include "graph/edgelist.h"
#include "graph/adjlist.h"
#include "disjoint_sets/union_find_array.h"


void kruskal_init(edgelist_t *el,
                  /*adjlist_t *al,*/
                  union_find_node_t **fnode_array,
                  int **edge_membership);

void kruskal_new_init(edgelist_t *el,
                      /*adjlist_t *al,*/
                      union_find_node_t **fnode_array,
                      edge_t ***msf);
                      //int **edge_membership);

void kruskal_init_shrt(edgelist_t *el,
                  /*adjlist_t *al,*/
                  union_find_node_t **fnode_array,
                  short int **edge_membership);

void kruskal_init_char(edgelist_t *el,
                  /*adjlist_t *al,*/
                  union_find_node_t **fnode_array,
                  char **edge_membership);

void kruskal_sort_edges(edgelist_t *el, int nthreads);
void kruskal_sort_edges_w_id(edgelist_w_id_t *el);

void kruskal(edgelist_t *el, 
             union_find_node_t *node_array,  
             int *edge_membership);

void kruskal_new(edgelist_t *el, 
                 union_find_node_t *node_array,  
                 edge_t **msf,
                 unsigned int *msf_edges);
                 //int *edge_membership)

void kruskal_w_id(edgelist_w_id_t *el, 
                  union_find_node_t *node_array,  
                  int *edge_membership);

unsigned int kruskal_up_to(edgelist_t *el,
                   union_find_node_t *array,
                   int *edge_membership,
                   unsigned int from,
                   unsigned int upto);

void kruskal_up_to_prepare(edgelist_t *el,
                   int *edge_membership,
                   unsigned int upto);

void kruskal_shrt(edgelist_t *el, 
             union_find_node_t *array,
             short int *edge_membership);

void kruskal_char(edgelist_t *el, 
             union_find_node_t *array,
             char *edge_membership);

unsigned int kruskal_get_msf_edge_count_char(edgelist_t *el,
                                             char *edge_membership);

weight_t kruskal_get_msf_weight_char(edgelist_t *el, char *edge_membership);

unsigned int kruskal_oracle(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership);

unsigned int kruskal_oracle_jump(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership);

unsigned int kruskal_oracle_shrt(edgelist_t *el, 
             union_find_node_t *array,
             short int *edge_membership);

unsigned int kruskal_oracle_jump_shrt(edgelist_t *el, 
             union_find_node_t *array,
             short int *edge_membership);

unsigned int kruskal_oracle_char(edgelist_t *el, 
             union_find_node_t *array,
             char *edge_membership);

unsigned int kruskal_oracle_jump_char(edgelist_t *el, 
             union_find_node_t *array,
             char *edge_membership);

void kruskal_oracle_prepare(edgelist_t *el,
                            union_find_node_t *array, 
                            int *edge_membership);

void kruskal_oracle_prepare_alt(edgelist_t *el,
                            union_find_node_t *array, 
                            int *edge_membership);

void kruskal_oracle_prepare_shrt(edgelist_t *el,
                            union_find_node_t *array, 
                            short int *edge_membership);

void kruskal_oracle_prepare_char(edgelist_t *el,
                            union_find_node_t *array, 
                            char *edge_membership);

void kruskal_oracle_jump_prepare(edgelist_t *el,
                            union_find_node_t *array, 
                            int *edge_membership);

void kruskal_oracle_jump_prepare_shrt(edgelist_t *el,
                                 union_find_node_t *array, 
                                 short int *edge_membership);

void kruskal_oracle_jump_prepare_char(edgelist_t *el,
                                 union_find_node_t *array, 
                                 char *edge_membership);

void kruskal_oracle_comp(edgelist_t *el, 
             union_find_node_t *array,
             int *edge_membership, edge_t *edge_array, int end);

int kruskal_oracle_prepare_comp(edgelist_t *el,
                                  union_find_node_t *array,
                                  int *edge_membership,
                                  edge_t *edge_array);

unsigned int kruskal_oracle_comp_mt(edgelist_t *el, 
                                    union_find_node_t *array,
                                    int *edge_membership,
                                    edge_t *edge_array);

void /*int*/ kruskal_oracle_prepare_comp_mt(edgelist_t *el,
                                  union_find_node_t *array,
                                  int *edge_membership,
                                  edge_t *edge_array,
                                  int begin, int end, int nthr);

void kruskal_ht_scheme_simulation(edgelist_t *unsorted_el,
                                  union_find_node_t *array,
                                  //int *edge_membership,
                                  int k, 
                                  /*unsigned int nedges,*/
                                  int main_threads,
                                  int helper_threads,
                                  edge_t **result,
                                  unsigned int *msf_edges);

void kruskal_destroy(/*adjlist_t *al,*/
                     union_find_node_t *fnode_array, 
                     //edge_t **msf);
                     void *edge_membership);

void kruskal_new_destroy(/*adjlist_t *al,*/
                         union_find_node_t *fnode_array, 
                         edge_t **msf);
                         //void *edge_membership);
#endif

