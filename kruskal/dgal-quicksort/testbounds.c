#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

#include "graph/edgelist.h"
#include "quicksort.h"
#include "partition/partition.h"

#include "machine/tsc_x86_64.h"

inline
int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data)
{
    return edge_compare(&a, &b);
}

#define FILTER 20.0

inline
int filter_out(edge_t e)
{
    return ( e.weight > FILTER );
}


_TYPE_V generate_random_elem(long int keyrange)
{
    edge_t ret;

    ret.vertex1 = rand() % keyrange;
    int v;
    while ( (v = rand() % keyrange) == ret.vertex1 );
    ret.vertex2 = v;
    ret.weight = (weight_t)(rand() % keyrange);

    return ret;
}

static void * array_make_copy(long int len, size_t sz, void *arr)
{
    void *ret = calloc(len, sz);
    if (!ret) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    memcpy(ret, arr, len*sz);
    
    return ret;
}

int main (int argc, char *argv[])
{
    if ( argc < 3 ) {
        printf("usage: %s <nelem> <keyrange>\n", argv[0]);
        exit(1);
    }

    long int nelem = atol(argv[1]);
    long int keyrange = atol(argv[2]);

    _TYPE_V *array_d = malloc(nelem*sizeof(_TYPE_V));
                         if ( !array_d ) {
        fprintf(stderr, "%s: Allocation error :(\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    // generate random input array
    long int i;
    for ( i = 0; i < nelem; i++ )
        array_d[i] = generate_random_elem(keyrange);

    _TYPE_V *copy = array_make_copy(nelem,sizeof(_TYPE_V),array_d);
    qsort(copy,nelem,sizeof(_TYPE_V),edge_compare);

    _TYPE_V pivot_elem = copy[nelem/2];

    memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    long int split;

    tsctimer_t tim;
    double hz;

    //---serial base partition
    timer_clear(&tim);
    timer_start(&tim);

    split = quicksort_partition_serial_base(pivot_elem, copy, 0, nelem, NULL);

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "partition serial base  cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    printf("%ld (or %lf%%) elements smaller than or equal to pivot value %lf\n", split, (double)split/nelem*100, pivot_elem.weight);

    //---concurrent inplace partition
    timer_clear(&tim);
    timer_start(&tim);

    #pragma omp parallel
    {
        split = quicksort_partition_concurrent_inplace(pivot_elem, array_d, 0, nelem-1, NULL);
    }

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "partition_conc inplc   cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    printf("%ld (or %lf%%) elements smaller than or equal to pivot value %lf\n", split, (double)split/nelem*100, pivot_elem.weight);

    free(copy);
    free(array_d);

    return 0;
}
