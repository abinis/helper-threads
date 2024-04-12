#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "filter_kruskal.h"
#include "dgal-quicksort/quicksort.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
#endif

// needed for quicksort.o
inline
int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data)
{
    return edge_compare(&a, &b);
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

static int validate_copy(edge_t * a, edge_t * b, long int nelem)
{
    assert(a);
    assert(b);

    long int i;
    for ( i = 0; i < nelem; i++ )
        if ( !edge_equality_check(&a[i],&b[i]) )
            return 0;

    return 1;
}

int main (int argc, char *argv[])
{
    if ( argc < 3 ) {
        printf("Usage: %s <graphfile> <mode(0:single-threaded,1:concurrent)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int mode = atoi(argv[2]);
    // won't see cmd-line passed value, separate compilation unit!
    //printf("THRESHOLD %d\n", THRESHOLD);

    edgelist_t *el;
    char *input_method;

#ifdef PROFILE
    tsctimer_t tim;
    double hz;
#endif

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    el = edgelist_choose_input_method(argv[1],0,0,&input_method);
    //if ( strcmp(extension,"dump") == 0 )
    //    el = edgelist_load(argv[1]);
    //else
    //    el = edgelist_read(argv[1], 0, 0);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_%s          cycles:%lf seconds:%lf freq:%lf\n", 
                    input_method,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    free(input_method);
    //printf("nedges=%d\n", el->nedges);
    //edgelist_print(el);

    //edge_t *copy = array_make_copy(el->nedges, sizeof(edge_t), el->edge_array);
    //assert( validate_copy(el->edge_array,copy,el->nedges) );

    int i;
//    long int sum = 0;
//    double avg;
//
//    srand(time(NULL));
//
//    printf("benchmarking pivot_rand...\n");
//#ifdef PROFILE
//    timer_clear(&tim);
//    timer_start(&tim);
//#endif
//    for ( i = 0; i < 1000000; i++ ) {
//        long int ret = rand() % el->nedges;
//        sum += ret;
//    }
//#ifdef PROFILE
//    timer_stop(&tim);
//    hz = timer_read_hz();
//    fprintf(stdout, "median rand            cycles:%lf seconds:%lf freq:%lf\n", 
//                    timer_total(&tim),
//                    timer_total(&tim) / hz,
//                    hz );
//#endif
//    avg = (double)(sum) / 1000000.0;
//    printf("avg pivot index = %lf :) [value = %f]\n", avg, el->edge_array[(int)avg].weight);
//    printf("...done!\n");
//
//    sum = 0;
//    printf("benchmarking pivot_rand_median3...\n");
//#ifdef PROFILE
//    timer_clear(&tim);
//    timer_start(&tim);
//#endif
//    for ( i = 0; i < 1000000; i++ ) {
//        long int ret = median_of_rand_three(el->edge_array, 0, el->nedges-1);
//        sum += ret;
//    }
//#ifdef PROFILE
//    timer_stop(&tim);
//    hz = timer_read_hz();
//    fprintf(stdout, "median rand three      cycles:%lf seconds:%lf freq:%lf\n", 
//                    timer_total(&tim),
//                    timer_total(&tim) / hz,
//                    hz );
//#endif
//    avg = (double)(sum) / 1000000.0;
//    printf("avg pivot index = %lf :) [value = %f]\n", avg, el->edge_array[(int)avg].weight);
//    printf("...done!\n");
//
//    sum = 0;
//    printf("benchmarking pivot_rand_median3_xor...\n");
//#ifdef PROFILE
//    timer_clear(&tim);
//    timer_start(&tim);
//#endif
//    for ( i = 0; i < 1000000; i++ ) {
//        long int ret = median_of_rand_three_xor(el->edge_array, 0, el->nedges-1);
//        sum += ret;
//    }
// #ifdef PROFILE
//    timer_stop(&tim);
//    hz = timer_read_hz();
//    fprintf(stdout, "median rand three xor  cycles:%lf seconds:%lf freq:%lf\n", 
//                    timer_total(&tim),
//                    timer_total(&tim) / hz,
//                    hz );
//#endif
//    avg = (double)(sum) / 1000000.0;
//    printf("avg pivot index = %lf :) [value = %f]\n", avg, el->edge_array[(int)avg].weight);
//    printf("...done!\n");
//
//    qsort(el->edge_array, el->nedges, sizeof(edge_t), edge_compare);
//    printf("median value after qsorting = %f :)\n", el->edge_array[el->nedges/2].weight);

    union_find_node_t *fnode_array;
    edge_t **msf;
    unsigned int msf_edges;
    float msf_weight;

    // single-threaded filter-kruskal
    if ( mode == 0 ) {
        filter_kruskal_init(el, &fnode_array, &msf);
    
        printf("k=%f\n", 0.02*el->nedges);
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        //custom_filter_kruskal(el, fnode_array, 0.02*el->nedges, msf, &msf_edges);
        filter_kruskal(el, fnode_array, msf, &msf_edges);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "filter-kruskal         cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        printf("msf_edges=%u\n", msf_edges);
    
        //int i;
        msf_weight = 0.0;
        for ( i = 0; i < msf_edges; i++ ) {
            //edge_print((void*)msf[i]);
            msf_weight += msf[i]->weight;
        }
        printf("msf_weight=%f\n", msf_weight);
    
        filter_kruskal_destroy(fnode_array,msf);

    } else {
        // concurrent filter-kruskal
        //free(el->edge_array);
        //el->edge_array = copy;
    
        // maybe the same as filter_kruskal_init after all
        // (that is, if we don't use a _global fnode_array)
        filter_kruskal_concurrent_init(el, &fnode_array, &msf);
    
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        //custom_filter_kruskal(el, fnode_array, 0.02*el->nedges, msf, &msf_edges);
        filter_kruskal_concurrent(el, fnode_array, msf, &msf_edges);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "concurrent filter-k    cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        printf("msf_edges=%u\n", msf_edges);
    
        //int i;
        msf_weight = 0.0;
        for ( i = 0; i < msf_edges; i++ ) {
            //edge_print((void*)msf[i]);
            msf_weight += msf[i]->weight;
        }
        printf("msf_weight=%f\n", msf_weight);
    
        // cleanup!
        filter_kruskal_destroy(fnode_array,msf);
    }

    edgelist_destroy(el);

    return 0;
}
