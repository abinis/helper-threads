#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <assert.h>
#include <omp.h>

#include "filter_kruskal.h"
#include "dgal-quicksort/quicksort.h"
#include "cpp_functions/cpp_psort.h"

//static int float_compare(const void *x1, const void *x2)
//{
//    if ( *((float*)x1) < *((float*)x2) ) 
//        return -1;
//    else if ( *((float*)x1) == *((float*)x2) )
//        return 0;
//    else
//        return 1;
//}

// to use with filter_out below, for the concurrent version :)
// --> moved to .h file
union_find_node_t *fnode_array_global;

inline
int filter_out(edge_t e)
{
    //printf("filter_out checking (%d,%d)\n", e.vertex1, e.vertex2);
    return ( union_find_array_find_helper(fnode_array_global, e.vertex1)
             == union_find_array_find_helper(fnode_array_global, e.vertex2) );
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

void filter_kruskal_init(edgelist_t *el,
                         union_find_node_t **fnode_array,
                         edge_t ***msf)
{
    *fnode_array = union_find_array_init(el->nvertices);

    // at most nvertices-1 edges in the MSF :)
    *msf = (edge_t**)malloc((el->nvertices-1)*sizeof(edge_t*));
    if ( !msf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
}

void filter_kruskal_concurrent_init(edgelist_t *el,
                                    union_find_node_t **fnode_array,
                                    edge_t ***msf)
{
    *fnode_array = union_find_array_init(el->nvertices);
    // also set the global pointer this time :)
    // hmm... maybe not (?)
    fnode_array_global = *fnode_array;

    // at most nvertices-1 edges in the MSF :)
    *msf = (edge_t**)malloc((el->nvertices-1)*sizeof(edge_t*));
    if ( !msf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
}

static inline/*unsigned*/ int partition(edge_t *edge_array,
                       /*unsigned*/ int left,
                       /*unsigned*/ int right)
{
    // (random) pivot element has been placed in the first position :)
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
        while ( edge_array[++i].weight < pivot.weight );
        while ( edge_array[--j].weight > pivot.weight );
        if ( i < j ) {
            swap_edge(&edge_array[i],&edge_array[j]/*,sizeof(edge_t)*/);
        }
        else return j;
    }
}

// placeholder condition! 
__attribute__((always_inline))
static inline int kruskal_threshold(edgelist_t *el,
                                    int m)
{
    //if ( m < 1000 )
    //    return 1;
    if ( m <= THRESHOLD )
        return 1;
    //if ( m <= ( 2 << 12 ) )
    //    return 1;

    return 0;
}

inline int filter(edge_t *edge_array,
                  union_find_node_t *fnode_array,
                  int left,
                  int right)
{
    int end = left; 

    edge_t *pe;
    int set1, set2;
    int i;
    for ( i = left; i < right; i++ ) {
        pe = &(edge_array[i]);
        set1 = union_find_array_find_helper(fnode_array, pe->vertex1);
        set2 = union_find_array_find_helper(fnode_array, pe->vertex2);

        // vertices belong in different sets, bring the edge to the front :)
        if ( set1 != set2 )
            edge_array[end++] = edge_array[i];
    }

    return end;
}

static inline void kruskal(edgelist_t *el,
                           union_find_node_t *fnode_array,
                           int left,
                           int right,
                           edge_t **msf,
                           unsigned int *msf_edges)
{
    long int i;
    edge_t *pe;
    int set1, set2;

    // corner case: check the single edge provided :)
    if ( left == right ) {
        pe = &(el->edge_array[left]);
    
        set1 = union_find_array_find(fnode_array, pe->vertex1);
        set2 = union_find_array_find(fnode_array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(fnode_array, set1, set2);
            msf[*msf_edges] = pe;
            (*msf_edges)++;
        }

        return;
    }

#ifndef BASE_ISORT
#ifndef CPP_SORT
    qsort(el->edge_array+left, right-left, sizeof(edge_t), edge_compare);
#else
    //TODO find best BOOST single-threaded sort algorithm, or even...
    // provide *parallel* sorting for the base case?!
    cpp_sort_edge_arr(el->edge_array+left, right-left, 1);
#endif
#else 
    long int e, j;
    for ( e = left+1; e < right; e++) {
        //edge_t key = el->edge_array[e];
        j = e;
        while ( j > left && el->edge_array[j-1].weight > el->edge_array[j].weight ) {
            swap_edge(&(el->edge_array[j]),&(el->edge_array[j-1]));
            j--;
        }
    }

    //for ( e = left; e < length; e++ ) {
    //    edge_print(&(el->edge_array[e]));
    //    if ( e < length-1 && el->edge_array[e].weight > el->edge_array[e+1].weight )
    //        printf("edge out of order! :(\n");
    //}
#endif

    for ( i = left; i < right; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_array_find(fnode_array, pe->vertex1);
        set2 = union_find_array_find(fnode_array, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_array_union(fnode_array, set1, set2);
            msf[*msf_edges] = pe;
            (*msf_edges)++;
        }
    }
}

#ifdef PIVOT_MEDIAN3
/*static*/ inline int median_of_three(edge_t *edge_array, int left, int right)
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
/*static*/ inline int median_of_rand_three(edge_t *edge_array, int left, int right)
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

    //printf("                      median weight = %f\n", edge_array[median].weight);
    //float *rand_weights = malloc(3*sizeof(float));
    //rand_weights[0] = edge_array[pi0].weight;
    //rand_weights[1] = edge_array[pi1].weight;
    //rand_weights[2] = edge_array[pi2].weight;
    //printf("                    (pi0, pi1, pi2) = (%d, %d, %d)\n", pi0, pi1, pi2);
    //printf("                       rand_weights = (%f, %f, %f)\n", rand_weights[0], rand_weights[1], rand_weights[2]);
    //qsort(rand_weights, 3, sizeof(float), float_compare);
    //printf("(supposedly :P) sorted rand_weights = (%f, %f, %f)\n", rand_weights[0], rand_weights[1], rand_weights[2]);

    //assert( edge_array[median].weight == rand_weights[1] );

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
    } else if ( (edge_array[pi1].weight > edge_array[pi0].weight) ^
                (edge_array[pi1].weight > edge_array[pi2].weight) ) {
        median = pi1;
    }
    else {
        median = pi2;
    }

    //printf("                      median weight = %f\n", edge_array[median].weight);
    //float *rand_weights = malloc(3*sizeof(float));
    //rand_weights[0] = edge_array[pi0].weight;
    //rand_weights[1] = edge_array[pi1].weight;
    //rand_weights[2] = edge_array[pi2].weight;
    //printf("                    (pi0, pi1, pi2) = (%d, %d, %d)\n", pi0, pi1, pi2);
    //printf("                       rand_weights = (%f, %f, %f)\n", rand_weights[0], rand_weights[1], rand_weights[2]);
    //qsort(rand_weights, 3, sizeof(float), float_compare);
    //printf("(supposedly :P) sorted rand_weights = (%f, %f, %f)\n", rand_weights[0], rand_weights[1], rand_weights[2]);

    //assert( edge_array[median].weight == rand_weights[1] );

    return median;
}
#endif

void filter_kruskal_rec(edgelist_t *el,
                        union_find_node_t *fnode_array,
                        int left,
                        int right,
                        edge_t **msf,
                        unsigned int *msf_edges)
{
    //printf("left=%d  right=%d\n", left, right);
    // for convenience
    edge_t *edge_array = el->edge_array;

    if ( kruskal_threshold(el,right-left) )
        kruskal(el, fnode_array, left, right, msf, msf_edges);
    else {
        // pick a random element as pivot :)
#ifdef PIVOT_MEDIAN3
        int pivot_index = median_of_three(el->edge_array,left,right-1);
#elif defined PIVOT_RAND_MEDIAN3
        int pivot_index = median_of_rand_three(el->edge_array,left,right-1);
#elif defined PIVOT_RAND_MEDIAN3_XOR
        int pivot_index = median_of_rand_three_xor(el->edge_array,left,right-1);
#else
        int pivot_index = ((int)rand()) % (right-left) + left;
#endif

        swap_edge(&edge_array[left],&edge_array[pivot_index]);

        int split = partition(edge_array,left,right-1);

        filter_kruskal_rec(el,fnode_array,left,split+1,msf,msf_edges);
        int end = filter(el->edge_array,fnode_array,split+1,right);
        filter_kruskal_rec(el,fnode_array,split+1,end,msf,msf_edges);
    }
}

/** TODO
 *  edgelist, msf and msf_edges could be global
 * right index is exclusive!
 */
void filter_kruskal_concurrent_rec(edgelist_t *el,
                                   /*union_find_node_t *fnode_array,*/
                                   int left,
                                   int right,
                                   edge_t **msf,
                                   unsigned int *msf_edges)
{
    //printf("left=%d  right=%d\n", left, right);
    // for convenience
    //printf("%s left=%d right=%d\n", __FUNCTION__, left, right);
    edge_t *edge_array = el->edge_array;

    if ( kruskal_threshold(el,right-left) )
        kruskal(el, fnode_array_global, left, right, msf, msf_edges);
    else {
        // Important! the base case above also saves us from
        // dividing with zero ;) Thus, only the base kruskal
        // needs to take care of a left==right call

        // pick a random element as pivot :)
#ifdef PIVOT_MEDIAN3
        int pivot_index = median_of_three(el->edge_array,left,right-1);
#elif defined PIVOT_RAND_MEDIAN3
        int pivot_index = median_of_rand_three(el->edge_array,left,right-1);
#elif defined PIVOT_RAND_MEDIAN3_XOR
        int pivot_index = median_of_rand_three_xor(el->edge_array,left,right-1);
#else
        int pivot_index = ((int)rand()) % (right-left) + left;
#endif

        swap_edge(&edge_array[left],&edge_array[pivot_index]);

        int split;

        #pragma omp parallel
        {
            split  = quicksort_partition_concurrent_inplace(edge_array[left],edge_array,left,right-1,NULL);
        }

        if ( split > left )
            split -= 1;

        //printf("right part: %d-%d\n",left,split+1);
        //printf(" left part: %d-%d\n",split+1,right);
        filter_kruskal_concurrent_rec(el,/*fnode_array,*/left,split+1,msf,msf_edges);

        int end;

        //printf("filtering in [%d,%d]\n", split+1, right-1);
        #pragma omp parallel
        {
            end = quicksort_filter_concurrent_inplace(el->edge_array,split+1,right-1);
        }
        //end = quicksort_filter_serial(el->edge_array,split+1,right);

        //printf("         end=%d\n", end);
        //if ( end > split+1 )
        //    end -= 1;
        //end = filter(el->edge_array,fnode_array_global,split+1,right);
        //printf("adjusted end=%d\n", end);

        filter_kruskal_concurrent_rec(el,/*fnode_array,*/split+1,end,msf,msf_edges);
    }
}

void custom_filter_kruskal(edgelist_t *el,
                           union_find_node_t *fnode_array,
                           int k,
                           edge_t **msf,
                           unsigned int *msf_edges)
{
    //printf("cstm f-k right [%d, %d)\n", 0, k);
    filter_kruskal_rec(el,fnode_array,0,k,msf,msf_edges);

    int end = filter(el->edge_array,fnode_array,k,el->nedges);
    //printf("cstm f-k left  [%d, %d)\n", k, end);
    filter_kruskal_rec(el,fnode_array,k,end,msf,msf_edges);
}

void filter_kruskal(edgelist_t *el,
                    union_find_node_t *fnode_array,
                    edge_t **msf,
                    unsigned int *msf_edges)
{
#ifdef PIVOT_MEDIAN3
    printf("using median3 pivot :)\n");
#endif

    //printf("THRESHOLD %d\n", THRESHOLD);

    srand( time(NULL) );

    *msf_edges = 0;
    filter_kruskal_rec(el,fnode_array,0,el->nedges,msf,msf_edges);
}

void filter_kruskal_concurrent(edgelist_t *el,
                               union_find_node_t *fnode_array,
                               edge_t **msf,
                               unsigned int *msf_edges)
{
#ifdef PIVOT_MEDIAN3
    printf("using median3 pivot :)\n");
#endif

    //printf("THRESHOLD %d\n", THRESHOLD);

    srand( 0/*time(NULL)*/);

    *msf_edges = 0;
    filter_kruskal_concurrent_rec(el,/*fnode_array,*/0,el->nedges,msf,msf_edges);
}

void filter_kruskal_destroy(union_find_node_t *fnode_array,
                            edge_t **msf)
{
    union_find_array_destroy(fnode_array);
    free(msf);
}

