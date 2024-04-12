/**
 * @file
 * Kruskal driver program
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <omp.h>
#include "dgal-quicksort/quicksort.h"

#include "graph/graph.h"
#include "graph/adjlist.h"
#include "kruskal_array.h"
#include "edge_stats.h"
#include "filter-kruskal/filter_kruskal.h"
#include "cpp_functions/cpp_psort.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
#endif

//FILE * cycles_f;
//FILE * jumps_f;
__attribute__((always_inline))
int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data)
{
    return edge_compare(&a, &b);
}

edgelist_w_id_t *edge_array_append_id(edgelist_t *el)
{
    edgelist_w_id_t *ret;

    ret = malloc(sizeof(edgelist_w_id_t));
    if (!ret) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->edge_array = (edge_w_id_t*)calloc(el->nedges, sizeof(edge_w_id_t));
    if ( !(ret->edge_array) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->nvertices = el->nvertices;
    ret->nedges = el->nedges;
    ret->is_undirected = el->is_undirected;
    
    unsigned int i;
    for ( i = 0; i < el->nedges; i++ ) {
        ret->edge_array[i].vertex1 = el->edge_array[i].vertex1;
        ret->edge_array[i].vertex2 = el->edge_array[i].vertex2;
        ret->edge_array[i].weight = el->edge_array[i].weight;
        ret->edge_array[i].id = i;
    }
   
    return ret;
}

edge_t* array_make_copy(edge_t *array, int left, int right)
{
    edge_t *ret;
    int len = right-left;

    ret = calloc(len,sizeof(edge_t));
    if (!ret) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    memcpy(ret, array, len*sizeof(edge_t));
    
    return ret;
}

static inline void swap_edge(edge_t *e1, edge_t *e2)
{
    edge_t tmp;

    tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
    
    //tmp.vertex1 = e1->vertex1;
    //tmp.vertex2 = e1->vertex2;
    //tmp.weight = e1->weight;

    //e1->vertex1 = e2->vertex1;
    //e1->vertex2 = e2->vertex2;
    //e1->weight = e2->weight;

    //e2->vertex1 = tmp.vertex1;
    //e2->vertex2 = tmp.vertex2;
    //e2->weight = tmp.weight;

    //memcpy(&tmp, e1, sizeof(edge_t));
    //memcpy(e1, e2, sizeof(edge_t));
    //memcpy(e2, &tmp, sizeof(edge_t));
}
/**
 * @return (j) the index of the last element smaller than the pivot,
 * i.e, elemens in the range [left,j] (notice j is inclusive)
 * are < than the pivot element; elements in [j+1,right] are >=
 * pivot element (again, right is also inclusive)
 */
static inline/*unsigned*/ int partition(edge_t *edge_array,
                              //int *edge_color,
                       /*unsigned*/ int left,
                       /*unsigned*/ int right)
{
    // pick the element in the first position as the pivot :)
    edge_t pivot;
    //pivot.vertex1 = edge_array[left].vertex1;
    //pivot.vertex2 = edge_array[left].vertex2;
    //pivot.weight = edge_array[left].weight;
    pivot = edge_array[left];

    //edge_print(&pivot);
   
    // a bit ugly :( since it's an unsigned int we are possibly reducing
    // below zero (underflow), but at least it's incremented right up to
    // zero before being used as an index :)
    /*unsigned*/ int i = left-1;
    /*unsigned*/ int j = right+1;

    while (1) {
        while ( /*edge_compare(&*/edge_array[++i].weight < pivot.weight/*,&pivot) < 0*/ );
        while ( /*edge_compare(&*/edge_array[--j].weight > pivot.weight/*,&pivot) > 0*/ );
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

#ifdef PIVOT_MEDIAN3
static inline int median_of_three(edge_t *edge_array, int left, int right)
{
    int min, max, median;
    int mid = (right-left)/2 + left;
    //printf("%s left=(%d,%f), mid=(%d,%f), right=(%d,%f) ", __FUNCTION__,
    //                                                        left, edge_array[left].weight,
    //                                                        mid, edge_array[mid].weight,
    //                                                        right, edge_array[right].weight );
    if ( edge_array[left].weight < edge_array[mid].weight ) {
        min = left;
        max = mid;
    } else {
        min = mid;
        max = left;
    }
    if ( edge_array[min].weight > edge_array[right].weight )
        median = min;
    else {
        if ( edge_array[max].weight < edge_array[right].weight )
            median = max;
        else
            median = right;
    }

    //printf("median=%d\n", median);

    return median;
}

#elif defined PIVOT_RAND_MEDIAN3
static inline int median_of_rand_three(edge_t *edge_array, int left, int right)
{
    if ( right-left < 3 )
        return ((int)rand()) % (right-left+1) + left;

    int min, max, median;

    // randomly choose three (different) pivot indexes :)
    int pi0, pi1, pi2;
    pi0 = ((int)rand()) % (right-left+1) + left;
    do {
        pi1 = ((int)rand()) % (right-left+1) + left;
    } while ( pi1 == pi0 );
    do {
        pi2 = ((int)rand()) % (right-left+1) + left;
    } while ( pi2 == pi0 || pi2 == pi1 );

    //printf("%s left=(%d,%f), mid=(%d,%f), right=(%d,%f) ", __FUNCTION__,
    //                                                        left, edge_array[left].weight,
    //                                                        mid, edge_array[mid].weight,
    //                                                        right, edge_array[right].weight );
    if ( edge_array[pi0].weight < edge_array[pi1].weight ) {
        min = pi0;
        max = pi1;
    } else {
        min = pi1;
        max = pi0;
    }
    if ( edge_array[min].weight > edge_array[pi2].weight )
        median = min;
    else {
        if ( edge_array[max].weight < edge_array[pi2].weight )
            median = max;
        else
            median = pi2;
    }

    //printf("median=%d\n", median);

    return median;
}

#elif defined PIVOT_RAND_MEDIAN3_XOR
/*static*/ inline int median_of_rand_three_xor(edge_t *edge_array, int left, int right)
{
    if ( right-left < 3 )
        return ((int)rand()) % (right-left+1) + left;

    int median;

    // randomly choose three (different) pivot indexes :)
    int pi0, pi1, pi2;
    pi0 = ((int)rand()) % (right-left+1) + left;
    do {
        pi1 = ((int)rand()) % (right-left+1) + left;
    } while ( pi1 == pi0 );
    do {
        pi2 = ((int)rand()) % (right-left+1) + left;
    } while ( pi2 == pi0 || pi2 == pi1 );

    //printf("%s left=(%d,%f), mid=(%d,%f), right=(%d,%f) ", __FUNCTION__,
    //                                                        left, edge_array[left].weight,
    //                                                        mid, edge_array[mid].weight,
    //                                                        right, edge_array[right].weight );
    if ( (edge_array[pi0].weight < edge_array[pi1].weight) ^
         (edge_array[pi0].weight < edge_array[pi2].weight) ) {
        median = pi0;
    } else if ( (edge_array[pi1].weight < edge_array[pi0].weight) ^
                (edge_array[pi1].weight < edge_array[pi2].weight) ) {
        median = pi1;
    }
    else {
        median = pi2;
    }

    //printf("median=%d\n", median);

    return median;
}
#endif

#ifdef CONCURRENT_PARTITION
edge_t *buf;
#endif

__attribute__((always_inline))
static inline int threshold(int m)
{
    //if ( m < 1000 )
    //    return 1;
    if ( m <= THRESHOLD )
        return 1;
    //if ( m <= ( 2 << 12 ) )
    //    return 1;

    return 0;
}

/*unsigned int*/void randomized_partition_around_k(edge_t *edge_array,
                                                //int *edge_color,
                                   /*unsigned*/ int left,
                                   /*unsigned*/ int right,
                                   /*unsigned*/ int k)
{
        
    if ( left == right )
        return;
        //return left;

#ifdef PIVOT_MEDIAN3
    int pivot = median_of_three(edge_array,left,right);
#elif defined PIVOT_RAND_MEDIAN3
    int pivot = median_of_rand_three(edge_array,left,right);
#elif defined PIVOT_RAND_MEDIAN3_XOR
    int pivot = median_of_rand_three_xor(edge_array,left,right);
#else
    // don't foget to add the offset (+left)!
    /*unsigned*/ int pivot = ((/*unsigned*/ int)(rand()) % (right-left+1)) + left;
#endif

    swap_edge(&edge_array[left],&edge_array[pivot]/*,sizeof(edge_t)*/);
    // also swap corresponding color :)
    //int temp = edge_color[left];
    //edge_color[left] = edge_color[pivot];
    //edge_color[pivot] = temp;

    // randomly partition the array around element @ pivot (which is now
    // at the left-most position due to the swap above)
#ifndef CONCURRENT_PARTITION
#ifndef CPP_PARTITION
    /*unsigned*/ int q = partition(edge_array/*,edge_color*/,left,right);
#else
    int q = cpp_partition_edge_arr(edge_array+left,right-left+1,0);
    q += left;
    if ( q > left )
        q -= 1;
#endif
#else
    int q;
    if ( threshold(right-left+1) ) {
        q = partition(edge_array/*,edge_color*/,left,right);
    }
    else {
        #pragma omp parallel
        {
            q = quicksort_partition_concurrent_inplace(edge_array[left], edge_array, left, right, NULL);
            //q = quicksort_partition_concurrent(edge_array, buf, left, right, NULL);
        }

        if ( q > left )
            q -= 1;
    }
#endif

//#ifndef CONCURRENT_PARTITION
//#ifndef CPP_PARTITION
    /*unsigned*/ int nel = q - left + 1;
//#else
//    int nel = q;
//#endif
//#else
//    int nel = q - left;
//#endif

    //printf("%s [%d,%d] q=%d nel=%d \n", __FUNCTION__, left, right, q, nel);

//#ifndef CONCURRENT_PARTITION
    if ( k <= nel ) return randomized_partition_around_k(edge_array/*,edge_color*/,left,q,k);
    else return randomized_partition_around_k(edge_array/*,edge_color*/,q+1,right,k-nel);
//#else
//    if ( k <= nel ) return randomized_partition_around_k(edge_array/*,edge_color*/,left,q-1,k);
//    else return randomized_partition_around_k(edge_array/*,edge_color*/,q,right,k-nel);
//#endif
}

int is_array_partitioned(edge_t *arr, size_t len, int upto)
{
    printf("%s split_index=%d split_val=%f\n", __FUNCTION__, upto, arr[upto].weight);

    int ret = 1;

    int i;
    for ( i = 0; i < upto; i++ ) {
        //printf("%9d %12f\n", i, arr[i].weight );
        if ( arr[i].weight > arr[upto].weight )
            ret = 0;
    }

    for ( i = upto; i < len; i++ ) {
        //printf("%9d %12f\n", i, arr[i].weight );
        if ( arr[i].weight < arr[upto-1].weight )
            ret = 0;
    }

    return ret;
}

int sim_validate(edgelist_t *el,
                 edge_t **msf,
                 unsigned int msf_edges,
                 edgelist_t *unsorted_el,
                 edge_t **sim_result,
                 unsigned int sim_msf_edgecount) 
{
    //int ret = 1;
    if ( msf_edges != sim_msf_edgecount )
        return 0;
        //ret = 0;

    int i;
    weight_t msf_weight = 0.0, sim_weight = 0.0;
    for ( i = 0; i < msf_edges; i++ ) {
        if ( msf[i]->weight != sim_result[i]->weight )
            return 0;
            //ret = 0;
        // probably redundant :P
        msf_weight += msf[i]->weight;
        sim_weight += sim_result[i]->weight;
    }

    //fprintf(stdout, "  Total MSF weight: %f\n", sim_weight);
    //fprintf(stdout, "   Total MSF edges: %24d\n", sim_msf_edgecount);
    if ( msf_weight != sim_weight )
        return 0;
        //ret = 0;

    //return ret;
    return 1;
}

int sim_validate_mst_weight_and_edgecount(edgelist_w_id_t *el,
                                      int *edge_membership,
                                      edgelist_t *unsorted_el,
                                      int *edge_membership_sim,
                                      edge_t **sim_result,
                                      unsigned int sim_msf_edgecount)
{
    int i;
    unsigned int msf_edgecount = 0, sim_edgecount = 0;
    float msf_weight = 0.0, sim_weight = 0.0;
    for ( i = 0; i < el->nedges; i++ ) {
        if ( edge_membership[i] == 1 ) {
            msf_weight += (el->edge_array[i]).weight;
            msf_edgecount++;
        }
        if ( edge_membership_sim[i] == 1 ) {
            //sim_weight += (unsorted_el->edge_array[i]).weight;
            sim_edgecount++;
        }
    }

    if ( /*msf_weight == sim_weight &&*/
         msf_edgecount != sim_edgecount )
        return 0;

    if ( sim_edgecount != sim_msf_edgecount )
        return 0;

    for ( i = 0; i < sim_msf_edgecount; i++ )
        sim_weight += sim_result[i]->weight;

    printf("msf_weight=%f, sim_weight=%f\n", msf_weight, sim_weight);

    if ( sim_weight != msf_weight )
        return 0;

    return 1;
}

// Actually used to make copies of other arrays, namely 
// the edge array itself :P
void * edge_membership_make_copy(unsigned int len, size_t sz, void *arr)
{
    void *ret = calloc(len, sz);
    if (!ret) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    memcpy(ret, arr, len*sz);
    
    return ret;
}

int validate_copy(unsigned int len, size_t sz,
                  void *arr, void *copy)
{
    if ( memcmp(arr, copy, len*sz) != 0 ) {
        printf("oops! bad copy :(\n");
        return 0;
    }
   
    return 1;
}

int edge_membership_check_result(unsigned int len,
                                 int *arr, int *copy)
{
    int i;
    for ( i = 0; i < len; i++ )
        if ( arr[i] == 1 && copy[i] != 1 )
            return 0;
   
    return 1;
}

int edge_membership_check_result_shrt(unsigned int len,
                                 short int *arr, short int *copy)
{
    int i;
    for ( i = 0; i < len; i++ )
        if ( arr[i] == 1 && copy[i] != 1 )
            return 0;
   
    return 1;
}

int edge_membership_check_result_char(unsigned int len,
                                 char *arr, char *copy)
{
    int i;
    for ( i = 0; i < len; i++ )
        if ( arr[i] == 1 && copy[i] != 1 )
            return 0;
   
    return 1;
}

int compression_check_result(edgelist_t* el,
                             int *edge_membership,
                             edge_t *edge_array_copy,
                             int *copy,
                             int end)
{
    int i, e = 0;
    for ( i = 0; i < el->nedges; i++ ) {
        if ( edge_membership[i] == 1 ) {
            if ( copy[e] != 1 )
                return 0;
            if (!edge_equality_check(&(el->edge_array[i]),&edge_array_copy[e]))
            {
                printf("%s: edges not equal! :O \n", __FUNCTION__);
                return 0;
            }
            e++;
        }
    }
    printf("%s: that's the correct edges!\n", __FUNCTION__);

    // Check if MSF edge count is correct
    if ( e != end )
        return 0;
    
    printf("%s: that's the correct edge count!\n", __FUNCTION__);

    // Also check positions of copy array beyond 'end' index, all values
    // should be negative!
    while ( e < el->nedges ) {
        if ( copy[e] >= 0 )
            return 0;
        e++;
    }

    printf("%s: last part of array is negative!\n", __FUNCTION__);

    return 1;
}

/*
void print_edge_stats(unsigned int len, int *edge_membership)
{
    int i;
    int num_datapoints = 100;
    double msf_edge_percentage;
    double cycle_edge_percentage;

    for ( i = 0; i < len; i++ ){}

}
*/

int main(int argc, char **argv) 
{
    int *edge_membership, 
                 e,
                 is_undirected;
    int next_option, print_flag, stats_flag, oracle_flag, sim_flag;
    char graphfile[256];
    // used (or not :P) with --simulate (shorthand 's') option
    //char kstring[32];
    double k;
    int nthreads, n_main_thr, n_helper_thr;
    //adjlist_stats_t stats;
    edgelist_t *el;
    //adjlist_t *al;
    union_find_node_t *fnode_array;
    unsigned int msf_edge_count;
    weight_t msf_weight;


    short int *edge_membership_shrt;
    char *edge_membership_char;

#ifdef PROFILE
    tsctimer_t tim;
    double hz;
#endif

    //Little piece of code to print the "usage..." help
    //message neatly :P
    char whitesp_pad[64];
    snprintf(whitesp_pad, 64, "Usage: %s", argv[0]);
    int lastchar = ( strlen(whitesp_pad) > 64 ) ? 63 : strlen(whitesp_pad);
    printf("lastchar = %d\n", lastchar);
    whitesp_pad[lastchar] = '\0';
    int i;
    for ( i = 0; i < lastchar; i++ )
        whitesp_pad[i] = ' ';

    if ( argc == 1 ) {
        printf("Usage: %s --graph <graphfile>\n"
                "%s [--print]\n"
                "%s [--stats]\n"
                "%s [--oracle]\n"
                "%s [--simulate <k:nthreads>]\n"
                  , argv[0], whitesp_pad, whitesp_pad, whitesp_pad, whitesp_pad);
        exit(EXIT_FAILURE);
    }

    print_flag = 0;
    stats_flag = 0;
    oracle_flag = 0;
    sim_flag = 0;

    /* getopt stuff */
    const char* short_options = "g:psom:";
    const struct option long_options[]={
        //{"interactive", 0, NULL, 'i'},
        {"simulate", 1, NULL, 'm'},
        {"oracle", 2, NULL, 'o'},
        {"stats", 2, NULL, 's'},
        {"graph", 1, NULL, 'g'},
        {"print", 0, NULL, 'p'},
        {NULL, 0, NULL, 0}
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, 
                                  NULL);
        switch(next_option) {
            case 'p':
                print_flag = 1;
                break;

            case 'g':
                sprintf(graphfile, "%s", optarg);
                break;
            
            //case 'i':
            //    break;

            case 's':
                stats_flag = 1;
                break;
            
            case 'o':
                oracle_flag = 1;
                break;

            case 'm':
                sim_flag = 1;
                //sprintf(kstring, "%s", optarg);
                printf("  optarg=%s\n", optarg);
                k = atof(strtok(optarg, ":"));
                if ( !(k > 0.0 && k < 1.0) ) {
                    printf("--simulate: argument 1 (k) should be in (0.0,1.0)!\n");
                    exit(EXIT_FAILURE);
                }
                printf("       k=%lf\n", k);
                char *nthr_str = strtok(NULL, ":");
                if ( nthr_str ) {
                    nthreads = atoi(nthr_str);
                    n_main_thr = 1;
                    n_helper_thr = nthreads-1;
                    //printf("nthreads=%d\n", nthreads);
                } else {
                    printf("--simulate: expected at least two (2) arguments\n"
                           "            delimited by ':', <k:nthreads>;\n"
                           "            or  three  (3),   <k:main:helper>\n");
                    exit(EXIT_FAILURE);
                }
                nthr_str = strtok(NULL, ":");
                if ( nthr_str ) {
                    n_main_thr = nthreads;
                    n_helper_thr = atoi(nthr_str);
                    nthreads += atoi(nthr_str);
                }
                printf("threads=(%d,%d:%d)\n",nthreads,n_main_thr,n_helper_thr);
                break;

            case '?':
                fprintf(stderr, "Unknown option!\n");
                exit(EXIT_FAILURE);

            case -1:    // Done with options
                break;  

            default:    // Unexpected error
                exit(EXIT_FAILURE);
        }

    } while ( next_option != -1 );

    // Init adjacency list
    //adjlist_init_stats(&stats);
    is_undirected = 0;
    char *inp_method;
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    //al = adjlist_read(graphfile, &stats, is_undirected);
    el = edgelist_choose_input_method(graphfile,is_undirected,0,&inp_method);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_%s          cycles:%lf seconds:%lf freq:%lf\n", 
                    inp_method,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
    free(inp_method);
    //printf("nvertices=%d\n", al->nvertices);
    fprintf(stdout, "Read graph\n\n");
    fprintf(stdout, "nedges=%u\n", el->nedges);

    //edgelist_w_id_t *el_w_id; 

    if ( sim_flag ) {

        printf("=== SIMULATE ===\n");

        printf("sim_flag#1 - init\n");

        edge_t **msf;
        unsigned int msf_edges = 0;
        edge_t *unsorted_edges, *unsorted_edges_copy;
        //el_w_id = edge_array_append_id(el);
        unsorted_edges = array_make_copy(el->edge_array, 0, el->nedges);

#ifdef CONCURRENT_PARTITION
        buf = malloc(el->nedges*sizeof(edge_t));
        if ( !buf ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }

        omp_set_num_threads(nthreads);
#endif

//#ifdef PROFILE
//    timer_clear(&tim);
//    timer_start(&tim);
//#endif
//    // Create edge list from adjacency list
//    el = edgelist_create(al);
//#ifdef PROFILE
//    timer_stop(&tim);
//    hz = timer_read_hz();
//    fprintf(stdout, "edgelist_create        cycles:%lf seconds:%lf freq:%lf\n", 
//                    timer_total(&tim),
//                    timer_total(&tim) / hz,
//                    hz );
//#endif
    
    // Run kruskal with 4 bytes (well, int :P) edge_membership
    // Then kruskal oracle
    // Then kruskal oracle with jumps 

        kruskal_new_init(el,/*al,*/ &fnode_array, &msf/*, &edge_membership*/);

    //if ( !sim_flag ) {
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
        kruskal_sort_edges(el,nthreads);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sort_edges             cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
    /*} else {
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    kruskal_sort_edges_w_id(el_w_id);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sort_edges             cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    }*/

    // Open file for cycle/jump storage ;)
    //cycles_f = fopen("cycles", "w");
    //jumps_f = fopen("jumps", "w");

    //if ( !sim_flag ) {
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

        kruskal_new(el, fnode_array, msf, &msf_edges/*, edge_membership*/);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal                cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
    /*} else {
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

    kruskal_w_id(el_w_id, fnode_array, edge_membership);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal                cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    }*/

    if ( print_flag )
        fprintf(stdout, "Edges in MSF:\n");

    msf_weight = 0.0;
    for ( e = 0; e < msf_edges; e++ ) {
        if ( print_flag )
            edge_print((void*)msf[e]);

        msf_weight += msf[e]->weight;
    }

    fprintf(stdout, "  Total MSF weight: %f\n", msf_weight);
    fprintf(stdout, "   Total MSF edges: %24d\n", msf_edges);


    // number of edges creating a cycle found
    //unsigned int cycles_found = 0;
    /*
    // number of edges not examined
    unsigned int not_examined = 0;

    weight_t msf_weight = 0.0;
    
    if ( print_flag )
        fprintf(stdout, "Edges in MSF:\n");

    if ( !sim_flag ) {

    for ( e = 0; e < el->nedges; e++ ) {
        //printf("%d", edge_membership[e]);
        if ( edge_membership[e] == 1 ) {
            msf_weight += el->edge_array[e].weight;
            msf_edge_count++;
            if ( print_flag ) {
                fprintf(stdout, "(%u,%u) [%.2f] \n", 
                        el->edge_array[e].vertex1,  
                        el->edge_array[e].vertex2,  
                        el->edge_array[e].weight);
            }
        } else if ( edge_membership[e] == 2 ) {
            cycles_found++;
        } else if ( edge_membership[e] == 0 ) {
            not_examined++;
        }
    }
    //printf("\n");

    } else {

    for ( e = 0; e < el_w_id->nedges; e++ ) {
        //printf("%d", edge_membership[e]);
        if ( edge_membership[e] == 1 ) {
            msf_weight += el_w_id->edge_array[e].weight;
            msf_edge_count++;
            if ( print_flag ) {
                fprintf(stdout, "#%u (%u,%u) [%.2f] \n", 
                        el_w_id->edge_array[e].id,
                        el_w_id->edge_array[e].vertex1,  
                        el_w_id->edge_array[e].vertex2,  
                        el_w_id->edge_array[e].weight);
            }
        } else if ( edge_membership[e] == 2 ) {
            cycles_found++;
        } else if ( edge_membership[e] == 0 ) {
            not_examined++;
        }
    }
    }
    //printf("\n");

    fprintf(stdout, "  Total MSF weight: %f\n", msf_weight);
    fprintf(stdout, "   Total MSF edges: %24d\n", msf_edge_count);
    fprintf(stdout, "Total cycles found: %24u\n", cycles_found);
    fprintf(stdout, "Edges not examined: %4u out of %12u\n", not_examined, el->nedges);
    */
    
    //if ( sim_flag ) {
        printf("sim_flag#2 - main!\n");

        // 1. Get the edge color for the unsorted edge array!
        /*
        int *unsorted_edge_color;
        unsorted_edge_color = calloc(el_w_id->nedges, sizeof(int));
        if (!unsorted_edge_color) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        
        for ( e = 0; e < el_w_id->nedges; e++ )
            unsorted_edge_color[el_w_id->edge_array[e].id] = edge_membership[e];

        printf("sim_flag#2 - unsorted color ok!\n");
        */

        // 2. Partition the unsorted edge array around k (given as 
        // argument to option --simulate !). This version of partitioning
        // also takes care of the ("parallel") unsorted_edge_color array :)
        assert( k > 0.0 && k < 1.0 );
        long int upto = lround(el/*_w_id*/->nedges * k) /*/ 100*/;
        printf("el->nedges*(1-k) = %lf or %ld or %lf\n", el->nedges*(1-k), lround(el->nedges*(1.0-k)), el->nedges*(1-k));
        int ht_region = el->nedges*(1-k);
        printf("ht_region = %d\n", ht_region);
        printf("upto=%ld\n", upto);

        srand( time(NULL) );
        
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif

#ifndef CPP_NTH_ELEMENT
        randomized_partition_around_k(unsorted_edges,
                                      //unsorted_edge_color,
                                      0,el/*_w_id*/->nedges-1,upto);
#else
        cpp_nth_element_edge_arr(unsorted_edges,el->nedges,upto);
#endif
        
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "sim - partition        cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif

#ifdef CONCURRENT_PARTITION
        free(buf);
#endif

        //printf("is array partitioned ? %c\n", is_array_partitioned(unsorted_edges, el->nedges, upto) ? 'y':'n');
        assert( is_array_partitioned(unsorted_edges, el->nedges, upto) );

        printf("sim_flag#2 - partition ok!\n");

        // make a copy of the newly partitioned edge_array, we'll need it below :)
        unsorted_edges_copy = array_make_copy(unsorted_edges, 0, el->nedges);

        //for ( i = 0; i < el->nedges; i++ )
        //    printf("(%d %d %f)\n", unsorted_edges[i].vertex1,
        //                           unsorted_edges[i].vertex2, 
        //                           unsorted_edges[i].weight);

        // 3. Run simulation on the now partitioned (still unsorted)
        // edge array, with help from the corresponding edge color array!
        // NOTE: we use the same fnode_array, we just zero it out
        edgelist_t *unsorted_el;
        unsorted_el = malloc(sizeof(edgelist_t));
        if (!unsorted_el) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        unsorted_el->nvertices = el->nvertices;
        unsorted_el->nedges = el->nedges;
        unsorted_el->edge_array = unsorted_edges;
        unsorted_el->is_undirected = el->is_undirected;

        // Reset union-find structure :)
        for (i = 0; i < unsorted_el->nvertices; i++) {
            fnode_array[i].parent = i;
            fnode_array[i].rank = 0;
        }

        // Create and initialize output array
        //int *edge_membership_sim = calloc(unsorted_el->nedges,sizeof(int));
        //if ( !edge_membership_sim ) {
        //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        //    exit(EXIT_FAILURE);
        //}


        // at most nvertices-1 edges in the MSF :)
        edge_t **sim_result = malloc((el->nvertices-1)*sizeof(edge_t*));
        if ( !sim_result ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        unsigned int sim_msf_edgecount = 0;


        printf("sim_flag#2 - checking custom filter-kruskal...\n");

        //for ( i = 0; i < el->nedges; i++ ) {
        //    printf("#%-3d ", i);
        //    edge_print(&unsorted_edges[i]);
        //}

    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        custom_filter_kruskal(unsorted_el,fnode_array,upto,sim_result,
                              &sim_msf_edgecount);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "sim - custom-filt-k    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif


        printf("sim ok? %c\n", sim_validate(el,
                                            msf,
                                            msf_edges,
                                            unsorted_el,
                                            sim_result,
                                            sim_msf_edgecount) ?
                               'y' : 'n');

    //if ( print_flag )
    //    fprintf(stdout, "Edges in MSF:\n");

    //weight_t sim_msf_weight = 0.0;
    //for ( e = 0; e < sim_msf_edgecount; e++ ) {
    //    if ( print_flag )
    //        edge_print((void*)sim_result[e]);

    //    sim_msf_weight += sim_result[e]->weight;
    //}

    //fprintf(stdout, "  Total MSF weight: %f\n", sim_msf_weight);
    //fprintf(stdout, "   Total MSF edges: %24d\n", sim_msf_edgecount);

        assert( sim_validate(el,msf,msf_edges,unsorted_el,sim_result,sim_msf_edgecount) );

        printf("sim_flag#2 - just before sim...\n");

        // Reset all that needs resetting XD
        free(unsorted_edges);
        unsorted_el->edge_array = unsorted_edges_copy;

        for (i = 0; i < unsorted_el->nvertices; i++) {
            fnode_array[i].parent = i;
            fnode_array[i].rank = 0;
        }

        free(sim_result);
        sim_result = malloc((el->nvertices-1)*sizeof(edge_t*));
        if ( !sim_result ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        sim_msf_edgecount = 0;


        kruskal_ht_scheme_simulation(unsorted_el,
                                     fnode_array,
                                     //edge_membership_sim,
                                     /*unsorted_edge_color,*/
                                     upto, /*el->nedges,*/
                                     n_main_thr,
                                     n_helper_thr,
                                     sim_result,
                                     &sim_msf_edgecount);

        // First, partition the edges
        //randomized_partition_around_k(unsorted_edges,0,el->nedges-1,10);
        // Second, qsort the leftmost part! (main thread)
        //qsort(unsorted_edges, 10, sizeof(edge_t), edge_compare);

        printf("sim_flag#2 - ...and after\n");

        //printf("sim ok? %c\n", sim_validate_mst_weight_and_edgecount(el_w_id,
        //                                             edge_membership,
        //                                             unsorted_el,
        //                                             edge_membership_sim,
        //                                             sim_result,
        //                                             sim_msf_edgecount) ?
        //                       'y' : 'n');


        printf("sim ok? %c\n", sim_validate(el,
                                            msf,
                                            msf_edges,
                                            unsorted_el,
                                            sim_result,
                                            sim_msf_edgecount) ?
                               'y' : 'n');

        assert( sim_validate(el,msf,msf_edges,unsorted_el,sim_result,sim_msf_edgecount) );

        free(sim_result);

        //free(unsorted_edge_color);
        //free(unsorted_edges);
        edgelist_destroy(unsorted_el);
        //free(edge_membership_sim);

        //free(el_w_id->edge_array);
        //free(el_w_id);

        kruskal_new_destroy(fnode_array, msf/*, edge_membership*/);
    }


    if ( stats_flag ) {

        printf("=== STATS ===\n");

        kruskal_init(el,/*al,*/ &fnode_array, &edge_membership);
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
        kruskal_sort_edges(el,1);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sort_edges             cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

        kruskal(el, fnode_array, edge_membership);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal                cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    // number of edges creating a cycle found
    unsigned int cycles_found = 0;
    
    // number of edges not examined
    unsigned int not_examined = 0;

    msf_edge_count = 0;
    msf_weight = 0.0;
    
    if ( print_flag )
        fprintf(stdout, "Edges in MSF:\n");

    for ( e = 0; e < el->nedges; e++ ) {
        //printf("%d", edge_membership[e]);
        if ( edge_membership[e] == 1 ) {
            msf_weight += el->edge_array[e].weight;
            msf_edge_count++;
            if ( print_flag )
                edge_print((void*)&(el->edge_array[e]));

        } else if ( edge_membership[e] == 2 ) {
            cycles_found++;
        } else if ( edge_membership[e] == 0 ) {
            not_examined++;
        }
    }

    fprintf(stdout, "  Total MSF weight: %f\n", msf_weight);
    fprintf(stdout, "   Total MSF edges: %24d\n", msf_edge_count);

        // get a zeroed-out copy of edge_membership :)
        int *copy = (int*)edge_membership_make_copy(el->nedges, sizeof(int),
                                                    edge_membership);

        assert(validate_copy(el->nedges, sizeof(int), edge_membership, copy));
        
        // also reset our union-find structure ;)
        for (i = 0; i < el->nvertices; i++) {
            fnode_array[i].parent = i;
            fnode_array[i].rank = 0;
        }

        unsigned int chunk = el->nedges / NUM_DATAPOINTS;
        int rem = el->nedges % NUM_DATAPOINTS;
        unsigned int from = 0;
        unsigned int upto;

        // compute the number of digits of 'len',
        // we need this for the field width of the unsigned ints below :)
        unsigned int width = 1;
        unsigned int number = el->nedges;
        while ( number / 10 > 0 ) {
            width++;
            number /= 10;
        }
        printf("width=%u\n", width);
        if ( width < 8 ) width = 8;

        for ( i = 1; i <= NUM_DATAPOINTS; i++ ) {
            upto = ( i <= rem ) ? i*(chunk+1) : rem*(chunk+1)+(i-rem)*chunk;

            //printf("from=%u upto=%u\n", from, upto);
            unsigned int ret = kruskal_up_to(el, fnode_array, copy, from, upto);
            //printf("ret=%u\n", ret);
            kruskal_up_to_prepare(el, copy, upto);

            // find how many cycles there are in [upto, el->nedges),
            // we need this to see how many of those we can offload :)
            unsigned int cycles_here = 0;
            for ( e = upto; e < el->nedges; e++ ) {
                // of course we use the original edge_membership array :)
                if ( edge_membership[e] == 2 )
                    cycles_here++;
            }

            printf("%3d %*u %*u %8.4lf%% %8.4lf%% %8.4lf%%\n", i, width, upto, width, ret,
                                       (double)ret / (double)cycles_found*100,
                                       (double)ret / (double)cycles_here*100,
                                       (double)ret / (double)(el->nedges-upto)*100);

            from = upto;
        }

        free(copy);

        print_edge_stats(el->nedges, edge_membership, msf_edge_count, cycles_found);
        print_gap_distribution(el->nedges, edge_membership);

        kruskal_destroy(fnode_array, edge_membership);
    }

    //FILE *edge_f = fopen("edges","w");
    //fprintf(edge_f, "    run: ");
    //for ( i = 0 ; i < el->nedges; i++ )
    //    fprintf(edge_f, "%d ", edge_membership[i]);
    //fprintf(edge_f, "\n");

    if ( oracle_flag ) {

        printf("=== ORACLE ===\n");

        kruskal_init(el,/*al,*/ &fnode_array, &edge_membership);
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
        kruskal_sort_edges(el,1);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "sort_edges             cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

        kruskal(el, fnode_array, edge_membership);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal                cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    // number of edges creating a cycle found
    unsigned int cycles_found = 0;
    
    // number of edges not examined
    unsigned int not_examined = 0;

    msf_edge_count = 0;
    msf_weight = 0.0;
    
    if ( print_flag )
        fprintf(stdout, "Edges in MSF:\n");

    for ( e = 0; e < el->nedges; e++ ) {
        //printf("%d", edge_membership[e]);
        if ( edge_membership[e] == 1 ) {
            msf_weight += el->edge_array[e].weight;
            msf_edge_count++;
            if ( print_flag )
                edge_print((void*)&(el->edge_array[e]));

        } else if ( edge_membership[e] == 2 ) {
            cycles_found++;
        } else if ( edge_membership[e] == 0 ) {
            not_examined++;
        }
    }

    fprintf(stdout, "  Total MSF weight: %f\n", msf_weight);
    fprintf(stdout, "   Total MSF edges: %24d\n", msf_edge_count);

        int *comp = (int*)edge_membership_make_copy(el->nedges, sizeof(int),
                                                    edge_membership);
        edge_t *edge_array_copy = (edge_t*)edge_membership_make_copy(el->nedges,
                                                    sizeof(edge_t),
                                                    el->edge_array);
        assert(validate_copy(el->nedges, sizeof(int), edge_membership, comp));
        assert(validate_copy(el->nedges, sizeof(edge_t), el->edge_array,
                             edge_array_copy));
        int end = kruskal_oracle_prepare_comp(el, fnode_array, comp,
                                              edge_array_copy);
        

    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        
        kruskal_oracle_comp(el, fnode_array, comp, edge_array_copy, end);
        
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_comp    cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        assert(cycles_found == el->nedges-end);
        // For this case ("compression"), a different assertion is needed,
        // since we modify the edge array itself :)
        assert(compression_check_result(el, edge_membership,
                                        edge_array_copy, comp, end));

                                        
        free(comp);
        free(edge_array_copy);


        comp = (int*)edge_membership_make_copy(el->nedges, sizeof(int),
                                                    edge_membership);
        edge_array_copy = (edge_t*)edge_membership_make_copy(el->nedges,
                                                    sizeof(edge_t),
                                                    el->edge_array);
        assert(validate_copy(el->nedges, sizeof(int), edge_membership, comp));
        assert(validate_copy(el->nedges, sizeof(edge_t), el->edge_array,
                             edge_array_copy));

          
        // Provisional values!
        int numhts = 8;
        int main_up_to = el->nedges / numhts;

        /*end =*/ kruskal_oracle_prepare_comp_mt(el, fnode_array, comp,
                                              edge_array_copy,
                                              0, el->nedges, numhts);
        // huh?
        //edgelist_print(el);

    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        
        unsigned int comp_mt_cycles = kruskal_oracle_comp_mt(el, 
                                                               fnode_array,
                                                               comp,
                                                               edge_array_copy);
        
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_comp_mt cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        assert(cycles_found == comp_mt_cycles);
        // For this case ("compression"), a different assertion is needed,
        // since we modify the edge array itself :)
        //assert(compression_check_result(el, edge_membership,
        //                                edge_array_copy, comp, end));

                                        
        free(comp);
        free(edge_array_copy);


        int *copy = (int*)edge_membership_make_copy(el->nedges, sizeof(int),
                                                    edge_membership);
        assert(validate_copy(el->nedges, sizeof(int), edge_membership, copy));
    
    //#ifdef PROFILE
    //    timer_clear(&tim);
    //    timer_start(&tim);
    //#endif
    
        kruskal_oracle_prepare(el, fnode_array, copy);
    
    //#ifdef PROFILE
    //    timer_stop(&tim);
    //    hz = timer_read_hz();
    //    fprintf(stdout, "oracle_prepare_bit     cycles:%lf seconds:%lf freq:%lf\n", 
    //                    timer_total(&tim),
    //                    timer_total(&tim) / hz,
    //                    hz );
    //#endif
    
        //fprintf(edge_f, "prepare: ");
        //for ( i = 0 ; i < el->nedges; i++ )
        //    fprintf(edge_f, "%d ", edge_membership[i]);
        //fprintf(edge_f, "\n");
    
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        
        unsigned int oracle_skip = kruskal_oracle(el, fnode_array, copy);
        
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle         cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        assert(cycles_found == oracle_skip);
        assert(edge_membership_check_result(el->nedges,
                                            edge_membership, copy));
    
        //fprintf(edge_f, " oracle: ");
        //for ( i = 0 ; i < el->nedges; i++ )
        //    fprintf(edge_f, "%d ", copy[i]);
        //fprintf(edge_f, "\n");
    
        kruskal_oracle_jump_prepare(el, fnode_array, copy);
    
        //fprintf(edge_f, " j_prep: ");
        //for ( i = 0 ; i < el->nedges; i++ )
        //    printf("%d ", copy[i]);
        //printf("\n");
    
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        
        unsigned int oracle_jump = kruskal_oracle_jump(el, fnode_array, copy);
        
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_jump    cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        //fprintf(edge_f, " j_orac: ");
        //for ( i = 0 ; i < el->nedges; i++ )
        //    printf("%d ", copy[i]);
        //printf("\n");
    
        assert(cycles_found == oracle_jump);
        assert(edge_membership_check_result(el->nedges, edge_membership, copy));
    
    //#ifdef PROFILE
    //    timer_clear(&tim);
    //    timer_start(&tim);
    //#endif
    
        // Repeat above, i.e. kruskal, kruskal oracle and kruskal oracle
        // with jumps, this time with 2-byte (short int) edge_membership
    
        // Clean-up first :)
        kruskal_destroy(/*al,*/ fnode_array, edge_membership);
        free(copy);
    
        kruskal_init_shrt(el, /*al,*/ &fnode_array, &edge_membership_shrt);
    
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
    
        kruskal_shrt(el, fnode_array, edge_membership_shrt);
    
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_shrt           cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
    
        short int *copy_shrt = (short int*)edge_membership_make_copy(el->nedges,
                                                    sizeof(short int),
                                                    edge_membership_shrt);
        assert(validate_copy(el->nedges, sizeof(short int), edge_membership_shrt, 
                                                          copy_shrt));
    
        kruskal_oracle_prepare_shrt(el, fnode_array, copy_shrt);
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        kruskal_oracle_shrt(el, fnode_array, copy_shrt);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_shrt    cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
        assert(edge_membership_check_result_shrt(el->nedges,
                                            edge_membership_shrt, copy_shrt));
    
        kruskal_oracle_jump_prepare_shrt(el, fnode_array, copy_shrt);
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        kruskal_oracle_jump_shrt(el, fnode_array, copy_shrt);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_jump_sh cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
        assert(edge_membership_check_result_shrt(el->nedges,
                                            edge_membership_shrt, copy_shrt));
    
        // For this last repeat, run
        // kruskal, kruskal oracle and kruskal oracle
        // with jumps, with 1-byte (char) edge_membership
    
        // Clean-up first :)
        kruskal_destroy(/*al,*/ fnode_array, edge_membership_shrt);
        free(copy_shrt);
    
        kruskal_init_char(el, /*al,*/ &fnode_array, &edge_membership_char);
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        kruskal_char(el, fnode_array, edge_membership_char);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_char           cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
        char *copy_char = (char*)edge_membership_make_copy(el->nedges,
                                                    sizeof(char),
                                                    edge_membership_char);
        assert(validate_copy(el->nedges, sizeof(char), edge_membership_char, 
                                                       copy_char));
    
        kruskal_oracle_prepare_char(el, fnode_array, copy_char);
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        kruskal_oracle_char(el, fnode_array, copy_char);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_char    cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
        assert(edge_membership_check_result_char(el->nedges,
                                            edge_membership_char, copy_char));
        kruskal_oracle_jump_prepare_char(el, fnode_array, copy_char);
    #ifdef PROFILE
        timer_clear(&tim);
        timer_start(&tim);
    #endif
        kruskal_oracle_jump_char(el, fnode_array, copy_char);
    #ifdef PROFILE
        timer_stop(&tim);
        hz = timer_read_hz();
        fprintf(stdout, "kruskal_oracle_jump_ch cycles:%lf seconds:%lf freq:%lf\n", 
                        timer_total(&tim),
                        timer_total(&tim) / hz,
                        hz );
    #endif
        assert(edge_membership_check_result_char(el->nedges,
                                            edge_membership_char, copy_char));
    
        // Clean-up!
        kruskal_destroy(/*al,*/ fnode_array, edge_membership_char);
        free(copy_char);

    }

    edgelist_destroy(el);
    //adjlist_destroy(al);

    return 0;
}

