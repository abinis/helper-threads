#ifndef DOUBLY_LINKED_LIST_
#define DOUBLY_LINKED_LIST_

#include "heap.h"

typedef struct doubly_linked_list_node_st {
    struct doubly_linked_list_node_st *next;
    struct doubly_linked_list_node_st *prev;
    edge_t *edge_array;
} doubly_linked_list_node_t;

typedef struct doubly_linked_list_header_st {
    doubly_linked_list_node_t *head;
    doubly_linked_list_node_t *tail;
    size_t size;
} doubly_linked_list_t;

doubly_linked_list_t * doubly_linked_list_create();
void doubly_linked_list_insert(doubly_linked_list_t *lp,
                               edge_t *arrp);
void doubly_linked_list_remove(doubly_linked_list_t *lp,
                               edge_t *arrp);
void doubly_linked_list_print(doubly_linked_list_t *lp);
void doubly_linked_list_destroy(doubly_linked_list_t *lp);

#endif
