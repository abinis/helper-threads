#include "edgelist.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#include <string.h>

//#include "util/util.h"
#include "machine/tsc_x86_64.h"

void array_print(edge_t *array, int len)
{
    int i;
    for ( i = 0; i < len; i++ )
       edge_print(&array[i]);
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

void swap_edge(edge_t *e1, edge_t *e2)
{
    edge_t tmp;

    
    tmp.vertex1 = e1->vertex1;
    tmp.vertex2 = e1->vertex2;
    tmp.weight = e1->weight;

    e1->vertex1 = e2->vertex1;
    e1->vertex2 = e2->vertex2;
    e1->weight = e2->weight;

    e2->vertex1 = tmp.vertex1;
    e2->vertex2 = tmp.vertex2;
    e2->weight = tmp.weight;

    //memcpy(&tmp, e1, sizeof(edge_t));
    //memcpy(e1, e2, sizeof(edge_t));
    //memcpy(e2, &tmp, sizeof(edge_t));
}

/*unsigned*/ int partition(edge_t *edge_array,
                       /*unsigned*/ int left,
                       /*unsigned*/ int right)
{
    // pick the element in the first position as the pivot :)
    edge_t pivot;
    pivot.vertex1 = edge_array[left].vertex1;
    pivot.vertex2 = edge_array[left].vertex2;
    pivot.weight = edge_array[left].weight;

    //edge_print(&pivot);
   
    // a bit ugly :( since it's an unsigned int we are possibly reducing
    // below zero (underflow), but at least it's incremented right up to
    // zero before being used as an index :)
    /*unsigned*/ int i = left-1;
    /*unsigned*/ int j = right+1;

    while (1) {
        while ( edge_compare(&edge_array[++i],&pivot) < 0 );
        while ( edge_compare(&edge_array[--j],&pivot) > 0 );
        if ( i < j ) swap_edge(&edge_array[i],&edge_array[j]/*,sizeof(edge_t)*/);
        else return j;
    }
}

/*unsigned*/ int randomized_partition_around_k(edge_t *edge_array,
                                   /*unsigned*/ int left,
                                   /*unsigned*/ int right,
                                   /*unsigned*/ int k)
{
    if ( left == right )
        return left;

    // don't foget to add the offset (+left)!
    /*unsigned*/ int pivot = ((/*unsigned*/ int)(rand()) % (right-left+1)) + left;

    swap_edge(&edge_array[left],&edge_array[pivot]/*,sizeof(edge_t)*/);

    // randomly partition the array around element @ pivot (which is now
    // at the left-most position due to the swap above)
    /*unsigned*/ int q = partition(edge_array,left,right);
    /*unsigned*/ int nel = q - left + 1;

    if ( k <= nel ) return randomized_partition_around_k(edge_array,left,q,k);
    else return randomized_partition_around_k(edge_array,q+1,right,k-nel);
}

/*unsigned int*/ void insertion_sort(edge_t *arr, int left, int right)
{
    //unsigned int ret = 0; 
    int i, j;

    //printf("left=%u right=%u\n", left, right);

    edge_t key;

    for ( j = left+1; j < right; j++ ) {
        key.vertex1 = arr[j].vertex1;
        key.vertex2 = arr[j].vertex2;
        key.weight = arr[j].weight;
        //printf("key @ %u copied!\n", j);
        i = j-1;
        //printf("i=%u j=%u\n", i, j);
        while ( i >= 0 && arr[i].weight > key.weight ) { 
            //ret++;
            //printf("accesing arr[%u]...\n", i);
            //while (arr[i].weight > key.weight/*edge_compare(&arr[i],&key) > 0*/) {
                arr[i+1].vertex1 = arr[i].vertex1;
                arr[i+1].vertex2 = arr[i].vertex2;
                arr[i+1].weight = arr[i].weight;
                i--;
            //}
        }
        arr[i+1].vertex1 = key.vertex1;
        arr[i+1].vertex2 = key.vertex2;
        arr[i+1].weight = key.weight;
    }

    //return ret;
}

int main (int argc, char *argv[])
{
    if ( argc < 5 ) {
        printf("usage: %s <graphfile> <is_undirected> <is_vertex_numbering_zero_based> <partition_pct>\n", argv[0]);
        exit(0);
    }

    printf("ok reading file...\n");

    printf("sizeof edge_t = %lu\n", sizeof(edge_t));

    edgelist_t* el = edgelist_read(argv[1], atoi(argv[2]), atoi(argv[3]));

    //edgelist_print(el);

    //edge_print(&(el->edge_array[0]));
    //edge_print(&(el->edge_array[10]));
    //swap(&(el->edge_array[0]),&(el->edge_array[10]),sizeof(edge_t));
    //swap_edge(&(el->edge_array[0]),&(el->edge_array[10]));

    //edge_print(&(el->edge_array[0]));
    //edge_print(&(el->edge_array[10]));

    
    int partition_pct = atoi(argv[4]);
    
    /*unsigned*/ int k = (/*unsigned*/ int) ((double)partition_pct * (double)el->nedges / 100.0);
    printf("k=%u\n", k);
    srand(time(NULL));

    /*unsigned*/ int hmm = randomized_partition_around_k(el->edge_array,0,el->nedges-1,k);
    printf("hmm=%u\n", hmm);
    
    //edgelist_print(el);

    /* random testing stuff :P */
    /*int e1, e2;
    edge_t foo;
    int cnt = 0;
    while (cnt++ < 100000) {
        e1 = rand() % el->nedges;
        e2 = rand() % el->nedges;

        foo.vertex1 = el->edge_array[e1].vertex1;
        foo.vertex2 = el->edge_array[e1].vertex2;
        foo.weight = el->edge_array[e1].weight;

        el->edge_array[e1].vertex1 = el->edge_array[e2].vertex1;
        el->edge_array[e1].vertex2 = el->edge_array[e2].vertex2;
        el->edge_array[e1].weight = el->edge_array[e2].weight;

        // if ( e1 == e2 ) {
        //     printf("break @ %d\n", e1);
        //     break;
        // }
    }*/

    tsctimer_t tim;
    double hz;
    
    edge_t *copy_ins = array_make_copy(el->edge_array, 0, k);

    timer_clear(&tim);
    timer_start(&tim);
    /*unsigned int comparisons =*/ insertion_sort(copy_ins, 0, k);
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "insertion sort      cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
    //printf("--insertion sort\n");
    array_print(copy_ins, k);

    edge_t *copy_qs = array_make_copy(el->edge_array, 0, k);

    timer_clear(&tim);
    timer_start(&tim);
    qsort(copy_qs, k, sizeof(edge_t), edge_compare);
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "quicksort           cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
    //printf("--quick sort\n");
    //array_print(copy_qs, k);


    //printf("insertion sort comparisons = %u\n", comparisons);
    //edgelist_print(el);

    edgelist_destroy(el);

    return 0;
}
