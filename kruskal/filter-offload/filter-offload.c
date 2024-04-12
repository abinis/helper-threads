#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#include "graph/edgelist.h"
#include "disjoint_sets/union_find_array.h"
#include "dgal-quicksort/quicksort.h"

extern edgelist_t *el;
extern union_find_node_t *array;
extern threshold;

extern edge_t **msf;
extern unsigned int msf_edges;

// placeholder condition! 
__attribute__((always_inline))
static inline
int kruskal_base(long int left, long int right)
{
    if ( m <= threshold )
        return 1;

    return 0;
}

void eat_chunk(long int index)
{

}

void filter_offload_rec(long int left,
                        long int right)
{
    if ( threshold(left,right) ) {
        kruskal_base(left,right);
        eat_chunk(right);
    }

    long int pivot_index = ((long int)rand()) % (right-left) + left;
    swap_edge(&(el->edge_array[left]),&(el->edge_array[pivot_index]));

    long int split;
    #pragma omp parallel
    {
        split = quicksort_partition_concurrent_inplace(el->edge_array[left],edge_array,left,right-1,NULL);
    }
    if ( split > left )
        split -= 1;

    //TODO spawn/OpenMP task
    filter(split,right);

    filter_offload_rec(left,split+1);

    //TODO sync/join eat_chunk
}

void filter_offload()
{
    
}
