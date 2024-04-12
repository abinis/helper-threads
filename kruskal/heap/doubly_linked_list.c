#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "doubly_linked_list.h"

doubly_linked_list_t * doubly_linked_list_create()
{
    doubly_linked_list_t * ret;

    ret = malloc(sizeof(doubly_linked_list_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->head = NULL;
    ret->tail = NULL;
    ret->size = 0;

    return ret;
}

void doubly_linked_list_insert(doubly_linked_list_t *lp, edge_t *arrp)
{
    assert(lp);

    doubly_linked_list_node_t *new;
    new = malloc(sizeof(doubly_linked_list_node_t));
    if ( !new ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    new->edge_array = arrp;

    // list empty
    if ( lp->head == NULL ) {
        lp->head = new;
        lp->tail = new;

        new->prev = NULL;
    }
    else {
        // we always add at the end! 
        lp->tail->next = new;
        new->prev = lp->tail;
        lp->tail = new;
    }

    new->next = NULL;

    (lp->size)++;
}

void doubly_linked_list_remove(doubly_linked_list_t *lp, edge_t *arrp)
{
    assert(lp);

    doubly_linked_list_node_t *np;

    np = lp->head;
    while ( np != NULL ) {
        // found it!
        if ( np->edge_array == arrp ) {
            if ( np->next != NULL )
                np->next->prev = np->prev;
            else // tail node!
                lp->tail = np->prev;

            if ( np->prev != NULL )
                np->prev->next = np->next;
            else // head node!
                lp->head = np->next;

            (lp->size)--;
            free(np->edge_array);
            free(np);

            break;
        }

        np = np->next;
    }
}

void doubly_linked_list_print(doubly_linked_list_t *lp)
{
    assert(lp);

    doubly_linked_list_node_t *np;

    np = lp->head;
    while ( np != NULL ) {
        printf("(%u %u %f)\n", (np->edge_array[0]).vertex1,
                               (np->edge_array[0]).vertex2,
                               (np->edge_array[0]).weight);
        np = np->next;
    }
}

void doubly_linked_list_destroy(doubly_linked_list_t *lp)
{
    assert(lp);

    doubly_linked_list_node_t *np1, *np2;
    
    np1 = lp->head;
    while ( np1 != NULL ) {
        np2 = np1->next;
        free(np1->edge_array);
        free(np1);
        np1 = np2;
    }
    
    free(lp);
}
