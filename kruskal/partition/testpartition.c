#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

#include "partition.h"

#include "machine/tsc_x86_64.h"

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

int int_compare (const void *a, const void *b)
{
    const int *ca = a;
    const int *cb = b;
    return (*ca > *cb) - (*ca < *cb);
}

int main (int argc, char *argv[])
{
    if ( argc < 3 ) {
        printf("usage: %s <k> <nelems>\n", argv[0]);
        exit(1);
    }

    int k = atoi(argv[1]);
    if ( k < 0 || k > 20 ) {
        printf("k (%d) must be within bounds!\n", k);
        exit(1);
    } 

    int array[20] = {13,15,2,8,9,1,11,6,4,12,17,19,3,5,10,7,14,18,16,0};

    //print_array((int*)array, 20);

    int el = randomized_partition_around_k((int*)array, 0, 19, k);

    //printf("el=%d\n", el);

    print_array((int*)array, 20);

    qsort(array, k, sizeof(int), int_compare);
    print_array((int*)array, 20);

    assert( el == k-1 );

    // generate random input elements!
    int nelem = atoi(argv[2]);

    double *array_d = malloc(nelem*sizeof(double));
    if ( !array_d ) {
        fprintf(stderr, "%s: Allocation error :(\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    unsigned int i;
    srand( time(NULL) );
    for ( i = 0; i < nelem; i++ )
        array_d[i] = (double)rand();

    unsigned int pivot = rand() % nelem;
    printf("pivot index = %u\n", pivot);

    tsctimer_t tim;
    double hz;

    timer_clear(&tim);
    timer_start(&tim);

    unsigned int ret = partition_double(array_d, 0, nelem-1, pivot);

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "partition_double       cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    printf("%u (or %lf%%) elements smaller than pivot %lf\n", ret, (double)ret/nelem*100, array_d[pivot]);

    free(array_d);

    return 0;
}
