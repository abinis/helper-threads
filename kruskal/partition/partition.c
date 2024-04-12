#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
//#include "partition.h"

#include "graph/edgelist.h"

static inline
void swap(void *x, void *y, size_t elem_size)
{
    char *_x, *_y, *tmp;

    _x = (char*)x;
    _y = (char*)y;

    tmp = malloc(elem_size);
    if ( !tmp ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    
    memcpy(tmp, _x, elem_size);
    memcpy(_x, _y, elem_size);
    memcpy(_y, tmp, elem_size);

    free(tmp);
}

static inline void swap_edge(edge_t *e1, edge_t *e2)
{
    edge_t tmp;

    tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
}

static inline void swap_double(double *x, double *y)
{
    double tmp = *x;
    *x = *y;
    *y = tmp;
}

unsigned int partition(int *edge_array,
                       unsigned int left,
                       unsigned int right)
{
    // pick the element in the last position as the pivot :)
    int pivot = edge_array[left];
   
    unsigned int i = left-1;
    unsigned int j = right+1;

    while (1) {
        while ( edge_array[++i] < pivot );
        while ( edge_array[--j] > pivot );
        if ( i < j ) swap(&edge_array[i],&edge_array[j],sizeof(int));
        else return j;
    }
}

long int partition_edge(edge_t *edge_array,
                            long int left,
                            long int right,
                            long int pivot)
{
    // pick the element in the first position as the pivot :)
    //edge_t pivot;
    //pivot.vertex1 = edge_array[left].vertex1;
    //pivot.vertex2 = edge_array[left].vertex2;
    //pivot.weight = edge_array[left].weight;
    //pivot = edge_array[left];

    // pick the element in the leftmost position as the pivot :)
    swap_edge(&edge_array[left],&edge_array[pivot]);
    edge_t pivot_elem = edge_array[left];
    //edge_print(&pivot);
   
    // a bit ugly :( since it's an unsigned int we are possibly reducing
    // below zero (underflow), but at least it's incremented right up to
    // zero before being used as an index :)
    /*unsigned*/ long int i = left-1;
    /*unsigned*/ long int j = right+1;

    while (1) {
        while ( /*edge_compare(&*/edge_array[++i].weight < pivot_elem.weight/*,&pivot) < 0*/ );
        while ( /*edge_compare(&*/edge_array[--j].weight > pivot_elem.weight/*,&pivot) > 0*/ );
        if ( i < j ) {
            swap_edge(&edge_array[i],&edge_array[j]/*,sizeof(edge_t)*/);
            // as always, also swap corresponding edge colors :)
            //int temp = edge_color[i];
            //edge_color[i] = edge_color[j];
            //edge_color[j] = temp;
        }
        else return j;
    }
}

long int partition_double(double *array,
                              long int left,
                              long int right,
                              long int pivot)
{
    // pick the element in the leftmost position as the pivot :)
    swap_double(&array[left],&array[pivot]);
    double pivot_elem = array[left];
   
    long int i = left-1;
    long int j = right+1;

    while (1) {
        while ( array[++i] < pivot_elem );
        while ( array[--j] > pivot_elem );
        if ( i < j ) swap_double(&array[i],&array[j]);
        else return j;
    }
}

void print_array( int *arr, int len )
{
    int i;
    printf("{");
    for ( i = 0; i < len; i++)
        printf("%2d%c", arr[i], (i < len-1)?',':'}');
    printf("\n");
}

int randomized_partition_around_k(int *edge_array,
                                   unsigned int left,
                                   unsigned int right,
                                   unsigned int k)
{
    if ( left == right )
        return edge_array[left];

    srand(time(NULL));
    // don't foget to add the offset!
    unsigned int pivot = ((unsigned int)(rand()) % (right-left+1)) + left;
    swap(&edge_array[left],&edge_array[pivot],sizeof(int));

    unsigned int q = partition(edge_array,left,right);
    unsigned int nel = q - left + 1;

    if ( k <= nel ) return randomized_partition_around_k(edge_array,left,q,k);
    else return randomized_partition_around_k(edge_array,q+1,right,k-nel);
}
