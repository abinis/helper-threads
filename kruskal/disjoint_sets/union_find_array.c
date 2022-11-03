/* disjoint-set set data structure implementation
 * using an ancestor array (insted of tree nodes/trees)
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


