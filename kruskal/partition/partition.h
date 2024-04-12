/**
 * @file
 * (Hopefully) things related to partitioning the input edge array
 * around some pivot value, finding the i-th smallest element and so on,
 * quickselect etc that is :D
 */

#ifndef PARTITION_H_
#define PARTITION_H_

/*
unsigned int partition(int *edge_array,
                       unsigned int left,
                       unsigned int right);
*/
void print_array( int *arr, int len );

int randomized_partition_around_k(int *edge_array,
                                   unsigned int left,
                                   unsigned int right,
                                   unsigned int k);

//long int partition_double(double *array,
//                              long int left,
//                              long int right,
//                              long int pivot);

long int partition(int *array,
                       long int left,
                       long int right,
                       long int pivot);

long int partition_edge(edge_t *edge_array,
                                         long int left,
                                         long int right,
                                         long int pivot);

#endif
