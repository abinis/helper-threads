#include <stdio.h>
#include "graph/edgelist.h"
#include "cpp_functions/cpp_psort.h"
#include "partition/partition.h"

int main (int argc, char *argv[])
{
    edge_t arr[3] = {{1,2,20.0},{4,3,40.0},{8,2,80.0}};

    //long int q = cpp_partition_edge_arr(arr,3,0);
    edge_t pivot = {42,71,96.0};
    long int q = cpp_partition_foo(arr,3,pivot);

    printf("cpp_partition_foo, pivot not in array! q = %ld\n", q);

    arr[0] = (edge_t){.vertex1=1,.vertex2=2,.weight=20.0};
    arr[1] = (edge_t){.vertex1=4,.vertex2=3,.weight=40.0};
    arr[2] = (edge_t){.vertex1=8,.vertex2=2,.weight=80.0};

    q = partition_edge(arr,0,2,0);

    printf("                  standard parition :) q = %ld\n", q);

    return 0;
}
