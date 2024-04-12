#ifndef HEAP_H_
#define HEAP_H_

#include "graph/edgelist.h"

/*
typedef struct edge_st {
    unsigned int vertex1;
    unsigned int vertex2;
    float weight;
} edge_t;
*/

typedef struct heap_node_st {
    void /*edge_t*/ *data;   // <- pointer to edge_t
    void /*edge_t*/ *maxv; // <- pointer to last elem of list/array :)
} heap_node_t;

typedef struct heap_aug_st {
    heap_node_t *heap;
    size_t hs;
    int (*comp_func)(const void *, const void *); 
} heap_aug_t;

typedef struct heap_st {
    void /*edge_t*/ **heap; // <- array of pointers to edge_t :)
    size_t hs;     // <- size of the heap!
    int (*comp_func)(const void *, const void *); // <- comparison function!
} heap_t;

void /*edge_t*/ * heap_delete_min(heap_t *hp);
void heap_insert(heap_t *hp, void /*edge_t*/ *key);
void /*edge_t*/ * heap_peek_min(heap_t *hp);
//void heap_construct(heap_t *hp);
heap_t * heap_init(void /*edge_t*/ *array, 
                   size_t elem_sz, 
                   size_t nelem,
                   int (*comp_func)(const void *, const void *));
heap_t * heap_create_empty(size_t nelem, 
                           int (*comp_func)(const void *, const void *));
int heap_is_empty(heap_t *hp);
void heap_print(heap_t *hp, void (*print_func)(void *));
void heap_destroy(heap_t *hp);

/* versions for the "augmented" heap :) */
heap_aug_t * heap_aug_init(void **data,
                           void **maxv,
                           size_t elemsz,
                           size_t nelem,
                           int (*comp_func)(const void *, const void *));
heap_aug_t * heap_aug_create_empty(size_t nelem, 
                                 int (*comp_func)(const void *, const void *));
void heap_aug_add(heap_aug_t *hp, void *data, void *maxv);
void heap_aug_construct(heap_aug_t *hp);
void * heap_aug_delete_min(heap_aug_t *hp);
void heap_aug_insert(heap_aug_t *hp, void /*edge_t*/ *key);
void heap_aug_increase_root_key(heap_aug_t *hp, void *new_key);
void * heap_aug_peek_min(heap_aug_t *hp);
int heap_aug_is_empty(heap_aug_t *hp);
void heap_aug_print(heap_aug_t *hp, void (*print_func)(void *));
void heap_aug_destroy(heap_aug_t *hp);


//int heap_edge_compare(const void *e1, const void *e2);

#endif
