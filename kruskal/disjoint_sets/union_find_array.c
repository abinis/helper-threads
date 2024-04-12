/* disjoint-set set data structure implementation
 * using an ancestor array (instead of tree nodes/trees)
 */

#include "union_find_array.h"

#include <stdio.h>
#include <stdlib.h>

union_find_node_t* union_find_array_init(int length)
{
    union_find_node_t* forest = malloc(length*sizeof(union_find_node_t));
    if ( !forest ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    int i;
    for (i = 0; i < length; i++) {
        forest[i].parent = i;
        forest[i].rank = 0;
    }
    
    return forest;
}

void union_find_array_destroy(union_find_node_t* forest)
{
    free(forest);
}

int union_find_array_find(union_find_node_t* arr, int index)
{
    int root = index;
    while ( arr[root].parent != root )
        root = arr[root].parent;

    int temp = index;
    int parent;
    while ( arr[temp].parent != root ) {
        //printf("find path compr: parent of %d change %d->%d\n", temp, arr[temp].parent, root);
        parent = arr[temp].parent;
        arr[temp].parent = root;
        temp = parent;
    }

    return root;
}

/**
 * find-set operation without path compression
 * @param hops the number of hops (==edges) to root is written
 * @return the root of the subtree where the node belongs to
 */
int union_find_array_find_hops(union_find_node_t* array, int index,
                               unsigned int *hops)
{
    // note: actually don't whether/how this is useful :P
    int root = index;
    *hops = 0;
    while ( array[root].parent != root ) {
        root = array[root].parent;
        (*hops)++;
    }
        
    return root;
}

/**
 * simply hangs the node @param new index from the node
 * @param parent index; that is, an edge new->...err, bad idea!
 */
void union_find_array_union_justhangit(union_find_node_t* arr, int new, 
                                       int parent)
{
    // doesn't work
}

void union_find_array_union(union_find_node_t* arr, int index1, int index2)
{
    if ( arr[index1].rank > arr[index2].rank ) {
        arr[index2].parent = index1;
    } else if ( arr[index2].rank > arr[index1].rank ) {
        arr[index1].parent = index2;
    } else { //equal
        arr[index2].parent = index1;
        (arr[index1].rank)++;
    }
}

/**
 * Find-set function for helper threads. Operates as the original 
 * find-set, without compressing the path
 * @param array the ancestor array maintained by the union-find data structure
 * @param index the node whose set we want to find
 * @return the root of the subtree where the node belongs to
 */ 
/*static inline*/
int union_find_array_find_helper(union_find_node_t* array, int index) 
{
    int root = index;
    while ( array[root].parent != root )
        root = array[root].parent;
        
    return root;
}

union_find_t * union_find_array_header_create(union_find_node_t *p, long s)
{
    union_find_t * ret;

    ret = malloc(sizeof(union_find_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->array = p;
    ret->size = s;

    return ret;
}

void union_find_array_header_destroy(union_find_t *p)
{
    free(p);
}

void union_find_array_print(union_find_t *ufp)
{
    long i;
    printf(" node->parent [rank]\n");
    for ( i = 0; i < ufp->size; i++ )
        printf(" %ld->%d [%d]\n", i, ufp->array[i].parent, ufp->array[i].rank);
}
