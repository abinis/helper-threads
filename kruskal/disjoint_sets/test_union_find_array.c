/**
 * @file
 * Simple program to test union-find array implementation/
 * compare with already tested (well, hopefully :P) tree 
 * implementation
 */

#include "union_find.h"
#include "union_find_array.h"

#include <stdio.h>
#include <stdlib.h>

void show_array (union_find_node_t* arr, int size)
{
    int i;
    for ( i = 0; i < size; i++ )
    {
        printf("%d [%d,%d]\n", i, arr[i].parent, arr[i].rank);
    }
}   

void do_union (union_find_node_t* arr, int index1, int index2)
{
    int set1, set2;
    set1 = union_find_array_find(arr, index1);
    set2 = union_find_array_find(arr, index2);
    union_find_array_union(arr, set1, set2);
    printf("union %d,%d done\n", index1, index2);
}

int main (int argc, char *argv[])
{
    int v = 10;

    //tree implementation
    forest_node_t** tree;
    //array implementtion
    union_find_node_t* array;

    /* Commented-out in order to check free at the bottom
     *
     * tree = (forest_node_t**)malloc(v*sizeof(forest_node_t*));
     * if ( !tree ) {
     *     fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
     *     exit(EXIT_FAILURE);
     * }

     * int i;
     * for ( i = 0; i < v; i++ )
     *     tree[i] = make_set(NULL);
     */

    //init ancestor array of length v
    array = union_find_array_init(v);

    //show contents (parents should be self, ranks equal to zero)
    show_array(array, v);

    //simple find test
    printf("find %d: %d\n", 5, union_find_array_find(array, 5));

    //let's see the example from the paper, a graph of 10 vertices
    //with a-j -> 0-9
    do_union(array, 0, 1);
    show_array(array, v);

    do_union(array, 6, 9);
    show_array(array, v);

    do_union(array, 8, 9);
    show_array(array, v);

    do_union(array, 3, 5);
    do_union(array, 0, 2);
    do_union(array, 1, 5);
    do_union(array, 2, 4);
    show_array(array, v);

    do_union(array, 5, 6);
    show_array(array, v);

    //test path compression for 5 ;)
    printf("find %d (path compr): %d\n", 5, union_find_array_find(array, 5));
    show_array(array, v);

    do_union(array, 4, 8);
    show_array(array, v);

    do_union(array, 5, 7);
    show_array(array, v);

    printf("finally, path compr for 9: %d\n", union_find_array_find(array, 9));
    show_array(array, v);

    //Finally, free allocated memory
    union_find_array_destroy(array);

    return 0;
}
