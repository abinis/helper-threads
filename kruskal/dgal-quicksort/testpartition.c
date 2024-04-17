#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <omp.h>

#include "graph/graph.h"
//#include "graph/edgelist.h"
#include "quicksort.h"
#include "partition/partition.h"
#include "cpp_functions/cpp_psort.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
// hmm that's kind of dodgy... i.e. assuming that you can warm up your
// cores and then frequency scaling becomes a non-issue
#include "time_it.h"
#endif

/*
void print_array( int *arr, int len )
{
    assert(arr);
    int i;
    printf("{");
    for ( i = 0; i < len; i++)
        printf("%2d%c", arr[i], (i < len-1)?',':'}');
    printf("\n");
}
*/

//__attribute__((always_inline))
inline
int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data)
{
    //if ( a < b )
    //    return -1;
    //else if ( a > b )
    //    return 1;
    //else
    //    return 0;
    return edge_compare(&a, &b);
}

#define FILTER 20.0

inline
int filter_out(edge_t e)
{
    return ( e.weight > FILTER );
}

int int_compare (const void *a, const void *b)
{
    const int *ca = a;
    const int *cb = b;
    return (*ca > *cb) - (*ca < *cb);
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

static int validate_copy(_TYPE_V * a, _TYPE_V * b, long int nelem)
{
    assert(a);
    assert(b);

    long int i;
    for ( i = 0; i < nelem; i++ )
        if ( !edge_equality_check(&a[i],&b[i]) )
            return 0;

    return 1;
}

//int array_is_partitioned(_TYPE_V *arr, size_t len, _TYPE_V pivot)
//{
//    long int i;
//    int mode = 0;
//
//    //printf("pivot = %14.2lf\n", pivot);
//    for ( i = 0; i < len; i++ ) {
//        //printf("        %14.2lf\n", arr[i]);
//        if ( arr[i].weight >= pivot.weight && mode == 0 )
//            mode = 1;
//        if ( arr[i].weight < pivot.weight && mode == 1 ) {
//            printf("oops! out-of-place element at index = %ld\n", i);
//            return 0;
//        }
//    }
//
//    return 1;
//}

// only change: ignore equality! elems equal to pivot could be anywhere :)
int array_is_partitioned_ignore_equal(_TYPE_V *arr, size_t len, _TYPE_V pivot)
{
    long int i;
    int mode = 0;

    //printf("pivot = %14.2lf\n", pivot);
    for ( i = 0; i < len; i++ ) {
        //printf("        %14.2lf\n", arr[i]);
        if ( arr[i].weight > pivot.weight && mode == 0 )
            mode = 1;
        if ( arr[i].weight < pivot.weight && mode == 1 )
            return 0;
    }

    return 1;
}

int main (int argc, char *argv[])
{
    //if ( argc < 2 ) {
    //    printf("usage: %s <nelems> [key_range]\n", argv[0]);
    //    exit(1);
    //}

    // generate random input elements!
    //long nelem = atol(argv[1]);
    //long key_range = RAND_MAX;
    //
    //if ( argc >= 3 )
    //    key_range = atol(argv[2]);

    if ( argc < 2 ) {
        printf("usage: %s <graphfile> [pivot_index(default:random)]\n", argv[0]);
        exit(1);
    }

    char graphfile[256];
    snprintf(graphfile, 256, "%s", argv[1]);

#ifdef PROFILE
    tsctimer_t tim;
    double hz;
#endif

    int is_undirected = 0;
    char *inp_method;

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

    //edgelist_t *el = edgelist_read(graphfile, 0, 0);
    edgelist_t *el = edgelist_choose_input_method(argv[1],is_undirected,0,&inp_method);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_%s          cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    inp_method,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    free(inp_method);

    long int nelem = el->nedges;
    _TYPE_V *array_d = el->edge_array;

    //_TYPE_V *copy;

    //_TYPE_V *array_d = malloc(nelem*sizeof(_TYPE_V));
    //if ( !array_d ) {
    //    fprintf(stderr, "%s: Allocation error :(\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}

    //_TYPE_V *buffer_d = malloc(nelem*sizeof(_TYPE_V));
    //if ( !buffer_d ) {
    //    fprintf(stderr, "%s: Allocation error :(\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}

    //long int i;
    //srand( time(NULL) );
    // ^^commented out, always produce same (pseudo)random input sequence
    //for ( i = 0; i < nelem; i++ ) {
    //    array_d[i] = (double)(rand() % key_range);
    //}


    // now the input is the same, choose a random pivot
    // ^^actually we can't, dgal implementations require middle element
    //   as pivot
    long int pivot_index;
    if ( argc >= 3 )
        pivot_index = atol(argv[2]);
    else {
        srand( time(NULL) );
        pivot_index = /*(nelem-1) / 2 */rand() % nelem;
    }
    weight_t pivot = array_d[pivot_index].weight;
    _TYPE_V pivot_elem = array_d[pivot_index];
    printf("pivot index = %ld, pivot = %lf\n", pivot_index, pivot);

    long int ret;

    //copy = array_make_copy(nelem, sizeof(_TYPE_V), array_d);
    //assert( validate_copy(array_d,copy,nelem) );

    // standard partition
    //timer_clear(&tim);
    //timer_start(&tim);

    //ret = partition_edge(copy, 0, nelem-1, pivot_index);

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "partition_edge         cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    //printf("%ld (or %lf%%) elements smaller than pivot %lf\n", ret+1, (double)(ret+1)/nelem*100, pivot);

    ////printf("array is partitioned ? %c\n", array_is_partitioned_ignore_equal(copy, nelem, pivot) ? 'y':'n' );
    //assert(array_is_partitioned_ignore_equal(copy, nelem, pivot_elem));

    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // c++ std::partition
    //timer_clear(&tim);
    //timer_start(&tim);

    //ret = cpp_partition_edge_arr(copy, nelem, pivot_index);

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "c++ std partition      cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    //printf("%ld (or %lf%%) elements smaller than pivot %lf\n", ret, (double)(ret)/nelem*100, pivot);

    ////printf("array is partitioned ? %c\n", array_is_partitioned_ignore_equal(copy, nelem, pivot) ? 'y':'n' );
    //assert(array_is_partitioned_ignore_equal(copy, nelem, pivot_elem));

    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // dgal serial partition
    //timer_clear(&tim);
    //timer_start(&tim);


    //ret = quicksort_partition_serial(copy, 0, nelem, NULL);

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "partition_serial       cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    //printf("%ld (or %lf%%) elements smaller than or equal to pivot %lf\n", ret, (double)ret/nelem*100, pivot);

    ////printf("array is partitioned ? %c\n", array_is_partitioned_ignore_equal(copy, nelem, pivot) ? 'y':'n' );
    //assert(array_is_partitioned_ignore_equal(copy, nelem, pivot_elem));

    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // dgal concurrent inplace partition
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

    #pragma omp parallel
    {
        ret = quicksort_partition_concurrent_inplace(pivot_elem, array_d/*copy*/, 0, nelem-1, NULL);
    }

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "partition_conc inplc   cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    printf("%ld (or %lf%%) elements smaller than or equal to pivot %lf\n", ret, (double)ret/nelem*100, pivot);
    assert(array_is_partitioned_ignore_equal(array_d/*copy*/, nelem, pivot_elem));

    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // dgal concurrent partition
    //timer_clear(&tim);
    //timer_start(&tim);

    //#pragma omp parallel
    //{
    //    ret = quicksort_partition_concurrent(copy, buffer_d, 0, nelem, NULL);
    //}

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "partition_concurrent   cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    //printf("%ld (or %lf%%) elements smaller than or equal to pivot %lf\n", ret, (double)ret/nelem*100, pivot);

    ////printf("array is partitioned ? %c\n", array_is_partitioned_ignore_equal(array_d, nelem, pivot) ? 'y':'n' );
    //assert(array_is_partitioned_ignore_equal(copy, nelem, pivot_elem));

    ////for ( i = 0; i < nelem; i++ )
    ////    printf("        %14.2lf %c\n", array_d[i], (array_d[i] != pivot) ? ((array_d[i] < pivot) ? '<':'>'):'=' );
    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // c++ std::nth_element
    //timer_clear(&tim);
    //timer_start(&tim);

    ///*edge_t *tmp =*/ cpp_nth_element_edge_arr(copy, nelem, nelem/2);

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "c++ std nth_element    cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    ////printf("%ld (or %lf%%) elements smaller than pivot %lf\n", ret, (double)(ret)/nelem*100, pivot);

    ////printf("array is partitioned ? %c\n", array_is_partitioned_ignore_equal(copy, nelem, pivot) ? 'y':'n' );
    //assert(array_is_partitioned_ignore_equal(copy, nelem, copy[nelem/2]));

    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // c++ std::sort
    //timer_clear(&tim);
    //timer_start(&tim);

    ///*ret =*/ cpp_sort_edge_arr(copy, nelem, 8);

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "c++ std sort 8 thr!    cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    ////printf("%ld (or %lf%%) elements smaller than pivot %lf\n", ret, (double)(ret)/nelem*100, pivot);

    ////printf("array is partitioned ? %c\n", array_is_partitioned_ignore_equal(copy, nelem, pivot) ? 'y':'n' );
    ////assert(array_is_partitioned_ignore_equal(tmp/*copy*/, nelem, tmp/*copy*/[nelem/2]));

    //memcpy(copy, array_d, nelem*sizeof(_TYPE_V));

    // concurrent inplace filtering
    //timer_clear(&tim);
    //timer_start(&tim);

    ////#pragma omp parallel
    ////{
    //    ret = quicksort_filter_serial(copy, 0, nelem);
    ////}

    //timer_stop(&tim);
    //hz = timer_read_hz();
    //fprintf(stderr, "filtr concurrent inpl  cycles:%lf seconds:%lf freq:%lf\n", 
    //                timer_total(&tim),
    //                timer_total(&tim) / hz,
    //                hz );

    //printf("%ld (or %lf%%) elements smaller than or equal to pivot %lf\n", ret, (double)ret/nelem*100, FILTER);
    ////assert(array_is_partitioned_ignore_equal(copy, nelem, pivot_elem));

    //int i;
    //for ( i = 0; i < nelem; i++ )
    //    edge_print(&copy[i]);

    //free(copy);

    //free(array_d);
    //free(buffer_d);

    edgelist_destroy(el);

    return 0;
}
