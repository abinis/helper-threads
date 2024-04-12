#ifndef CPP_FUNCTIONS_H_
#define CPP_FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

void cpp_sort_edge_arr(edge_t *arr, size_t len, int nthr);
long int cpp_partition_edge_arr(edge_t *arr, size_t len, long int pivot_index);
//long int cpp_partition_double_arr(double *, size_t, int);
long int cpp_partition_foo(edge_t * arr, size_t len, edge_t pivot);
void cpp_nth_element_edge_arr(edge_t *arr, size_t len, long int n);
edge_t * cpp_nth_element_edge_arr_v(edge_t *arr, size_t len, long int n);


#ifdef __cplusplus
    }
#endif

#endif
