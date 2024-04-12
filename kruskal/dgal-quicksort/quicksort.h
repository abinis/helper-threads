#ifndef QUICKSORT_H_
#define QUICKSORT_H_

#include "graph/edgelist.h"

#define _TYPE_V  edge_t
#define _TYPE_I  int
#define _TYPE_AD  void

long quicksort_partition_serial(_TYPE_V * A, long s, long e, _TYPE_AD * aux_data);
long quicksort_partition_concurrent(_TYPE_V * A, _TYPE_V * buf, long s, long e, _TYPE_AD * aux_data);
long quicksort_partition_concurrent_inplace(_TYPE_V pivot, _TYPE_V * A, long i_start, long i_end, _TYPE_AD * aux_data);
long quicksort_partition_serial_base(_TYPE_V pivot, _TYPE_V * A, long s, long e, _TYPE_AD * aux_data);
void quicksort_no_malloc(_TYPE_V * A, long N, _TYPE_AD * aux_data, _TYPE_I * partitions);
void quicksort(_TYPE_V * A, long N, _TYPE_AD * aux_data);

// includer must provide an implementation :)
extern int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data);

long quicksort_filter_serial(_TYPE_V * A, long s, long e);
long quicksort_filter_concurrent_inplace(_TYPE_V * A, long i_start, long i_end);
// includer must provide an implementation :)
extern int filter_out(_TYPE_V e);

#endif
