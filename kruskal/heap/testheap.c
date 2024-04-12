#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#include "heap.h"

#include "machine/tsc_x86_64.h"

int heap_validate_heap_property(heap_t *hp)
{
    assert(hp);

    int i;
    for ( i = 0; i < (hp->hs)/2; i++ ) 
        if ( ( /*heap_*/edge_compare(hp->heap[i],hp->heap[2*i+1]) > 0 ) ||
             ( ( 2*i+2 < hp->hs ) && ( /*heap_*/edge_compare(hp->heap[i],hp->heap[2*i+2]) > 0 ) ) )
            return 0;

    return 1;
}

int heap_aug_validate_heap_property(heap_aug_t *hp)
{
    assert(hp);

    int i;
    for ( i = 0; i < (hp->hs)/2; i++ ) 
        if ( ( edge_compare(hp->heap[i].data,hp->heap[2*i+1].data) > 0 ) ||
             ( ( 2*i+2 < hp->hs ) && ( edge_compare(hp->heap[i].data,hp->heap[2*i+2].data) > 0 ) ) )
            return 0;

    return 1;
}

int main (int argc, char *argv[])
{
    if ( argc < 3 ) {
        printf("usage: %s <heap_size> <nthreads>\n", argv[0]);
        exit(0);
    }

    edge_t **edge_array;
    size_t nedges = atoi(argv[1]);
    int nthr = atoi(argv[2]);

    edge_array = calloc(nthr,sizeof(edge_t*));
    if ( !edge_array ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    int i;

    for ( i = 0; i < nthr; i++ ) {
        // we allocate +1 for the guard element :)
        edge_array[i] = calloc(nedges+1,sizeof(edge_t));
        if ( !edge_array[i] ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
    }

    srand(time(NULL));

    int k;
    int val1, val2;
    float w;
    for ( k = 0; k < nthr; k++ ) {
        for ( i = 0; i < nedges; i++ ) {
            val1 = rand() % 100;
            edge_array[k][i].vertex1 = val1;
            //printf("(%d ", val1);
            val2 = rand() % 100;
            while ( val2 == val1 ) val2 = rand() % 100;
            edge_array[k][i].vertex2 = val2;
            //printf("%d ", val2);
            w = (float) (rand() % 1000 + 1.0);
            edge_array[k][i].weight = w;
            //printf("%f)\n", w);
        }

        qsort(edge_array[k], nedges, sizeof(edge_t), /*heap_*/edge_compare);
        // finally, add the guard element :)
        // (we can't use -1 due to unsigned int)
        edge_array[k][i].vertex1 = 0;
        edge_array[k][i].vertex2 = 0;
        edge_array[k][i].weight = -1.0;
        //printf("\n");
    }
    //printf("\n");
    //for ( k = 0; k < nthr; k++ ) {
    //    printf("%d\n", k);
    //    for ( i = 0; i < nedges; i++ ) {
    //        //edge_print(&edge_array[k][i]);
    //        printf("    (%d %d %f)\n", edge_array[k][i].vertex1,
    //                               edge_array[k][i].vertex2,
    //                               edge_array[k][i].weight);
    //    }
    //}


    edge_t *elem = &(edge_array[1][4]);
    int *color = calloc(nedges,sizeof(int));

    printf("offset=%ld (%lu in bytes)\n", elem-edge_array[1], sizeof(edge_t)*(elem-edge_array[1]));
    color[elem-edge_array[1]] = 1;
    printf("color=%d\n", color[4]);
    free(color);

    heap_t *hp = heap_init(edge_array[0], sizeof(edge_t), nedges, edge_compare);

    //printf("\n");

    //heap_construct(hp);

    //heap_print(hp, edge_print);

    assert(heap_validate_heap_property(hp));

    edge_t *minp = heap_peek_min(hp);
    //printf("%d %d %f\n", minp->vertex1, minp->vertex2, minp->weight);

    edge_t *mind = heap_delete_min(hp);
    //printf("%d %d %f\n", mind->vertex1, mind->vertex2, mind->weight);
    //heap_print(hp);
    //printf("\n");

    //assert( edge_compare(heap_peek_min(hp),heap_delete_min(hp)) == 0 );
    assert( /*heap_*/edge_compare(minp,mind) == 0 );

    assert(heap_validate_heap_property(hp));

    val1 = rand() % 100;
    //edge_array[hp->hs].vertex1 = val1;
    //printf("%d ", val1);
    val2 = rand() % 100;
    while ( val2 == val1 ) val2 = rand() % 100;
    //edge_array[hp->hs].vertex2 = val2;
    //printf("%d ", val2);
    w = (float) (rand() % 1000);
    //edge_array[hp->hs].weight = w;
    //printf("%f\n", w);

    edge_t *new = malloc(sizeof(edge_t));
    new->vertex1 = val1;
    new->vertex2 = val2;
    new->weight = w;

    heap_insert(hp, new);

    //heap_print(hp);
    //printf("\n");

    //heap_insert(hp, &(edge_array[2]));
    //heap_print(hp);

    assert(heap_validate_heap_property(hp));

    heap_destroy(hp);
    free(new);

#ifdef USE_HEAP

    printf("using heap...\n");

    /* BEGIN timed region */
    tsctimer_t tim;
    double hz;

    timer_clear(&tim);
    timer_start(&tim);

    //unsigned int touched = 0;

    hp = heap_create_empty(nthr, edge_compare);

    for ( i = 0; i < nthr; i++ )
        heap_insert(hp, &edge_array[i][0]);

    //heap_print(hp, edge_print);
 
    //assert(heap_validate_heap_property(hp));


    unsigned int touched = 0;

    //while ( hp->hs > 0 ) {
    while ( !heap_is_empty(hp) ) {
    //for ( i = 0; i < nthr*nedges; i++ ) {
        edge_t *min = heap_delete_min(hp);
        //printf("%f\n", min->weight);
        //assert(heap_validate_heap_property(hp));
        touched++;
        edge_t *next = (edge_t*)(min+1);
        if ( next->weight != -1.0 )
            heap_insert(hp, next);

        //assert(heap_validate_heap_property(hp));
    }


    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "heap traverse          cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    /* END timed region */

    // cleanup
    heap_destroy(hp);

    /* BEGIN timed region */

    timer_clear(&tim);
    timer_start(&tim);

    //TODO -DONE! These should be the *addresses* of the first/last elements!
    //void /*edge_t*/ **first = malloc(nthr*sizeof(void*));
    //if ( !first ) {
    //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}
    //void /*edge_t*/ **last = malloc(nthr*sizeof(void*));
    //if ( !last ) {
    //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}
    //for ( i = 0; i < nthr; i++ ) {
    //    first[i] = &(edge_array[i][0]);
    //    //printf("first: ");
    //    //edge_print(&first[i]);
    //    last[i] = &(edge_array[i][nedges-1]);
    //    //printf(" last: ");
    //    //edge_print(&last[i]);
    //}

    //printf("pointers taken!\n");

    //heap_aug_t *aughp = heap_aug_init(first, last, 
    //                                  sizeof(edge_t), nthr,
    //                                  edge_compare);

    heap_aug_t *aughp = heap_aug_create_empty(nthr, edge_compare);

    for ( i = 0; i < nthr; i++ ) {
        heap_aug_add(aughp, &(edge_array[i][0]), &(edge_array[i][nedges-1]));
    }

    heap_aug_construct(aughp);

    //printf("heap_aug_init ok!\n");

    //printf("heap:\n");
    //heap_aug_print(aughp, edge_print);

    //assert(heap_aug_validate_heap_property(aughp));

    //touched = 0;

    //printf("in order ->\n");
    while ( !heap_aug_is_empty(aughp) ) {
        edge_t *min = (edge_t*)heap_aug_delete_min(aughp);

        //printf("min: ");
        //edge_print(min);

        //printf("heap:\n");
        //heap_aug_print(aughp, edge_print);
        //touched++;
        edge_t *next = (edge_t*)(min+1);
        //printf("next_addr = %p\n", next);
        heap_aug_insert(aughp, next);
    }

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "heap_aug traverse      cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    /* END timed region */

    // cleanup
    heap_aug_destroy(aughp);

    //free(first);
    //free(last);

    //assert( touched == nthr*nedges );

    /* BEGIN timed region */

    timer_clear(&tim);
    timer_start(&tim);

    aughp = heap_aug_create_empty(nthr, edge_compare);

    //touched = 0;

    for ( i = 0; i < nthr; i++ ) {
        heap_aug_add(aughp, &(edge_array[i][0]), &(edge_array[i][nedges-1]));
    }

    heap_aug_construct(aughp);

    //assert(heap_aug_validate_heap_property(aughp));

    while ( !heap_aug_is_empty(aughp) ) {
        edge_t *min = (edge_t*)heap_aug_peek_min(aughp);

        //printf("min: ");
        //edge_print(min);

        //printf("heap:\n");
        //heap_aug_print(aughp, edge_print);
        //touched++;
        edge_t *next = (edge_t*)(min+1);
        //printf("next_addr = %p\n", next);
        heap_aug_increase_root_key(aughp, next);
        //assert(heap_aug_validate_heap_property(aughp));
    }

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "heap_aug incr key tr   cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    /* END timed region */

    //assert( touched == nthr*nedges );

    // cleanup
    heap_aug_destroy(aughp);

    for ( k = 0; k < nthr; k++ )
        free(edge_array[k]);
    free(edge_array);

#endif

#ifdef USE_FIND_MIN

    printf("using find_min...\n");

    tsctimer_t tim;
    double hz;

    timer_clear(&tim);
    timer_start(&tim);

    int *pos = calloc(nthr,sizeof(int));
    if ( !pos ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    int nlsts = nthr;
    edge_t *min;
    int min_pos;
    int j;
    unsigned int touched = 0;
    do /*for ( i = 0; i < nthr*nedges; i++ )*/ {

        min = &(edge_array[0][pos[0]]);
        min_pos = 0;

        for ( k = 1; k < nlsts; k++ ) {
            if ( edge_array[k][pos[k]].weight < min->weight ) {
                min = &(edge_array[k][pos[k]]);
                min_pos = k;
            }
        }
        touched++;
        //printf("%f, position %d within array %d\n", min->weight, pos[min_pos], min_pos);
        pos[min_pos] = pos[min_pos]+1;
            
        // get rid of the empty array!
        if ( edge_array[min_pos][pos[min_pos]].weight == -1.0 ) {
            free(edge_array[min_pos]);
            for ( j = min_pos+1; j < nlsts; j++ ) {
                edge_array[j-1] = edge_array[j];
                pos[j-1] = pos[j];
            }
            nlsts -= 1;
        }

        //assert(heap_validate_heap_property(hp));
    } while ( nlsts > 1 );

    // only one array left :)
    j = pos[0];
    while ( edge_array[0][j].weight != -1.0 ) {
        min = &(edge_array[0][j]);
        //printf("%f, position %d within array 0\n", min->weight, j);
        touched++;
        j++;
    }

    assert( touched == nthr*nedges );

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "findmin traverse       cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    free(edge_array[0]);
    free(edge_array);

    free(pos);

#endif

#ifdef OLD

    printf("using find_min (old)...\n");

    tsctimer_t tim;
    double hz;

    timer_clear(&tim);
    timer_start(&tim);

    int *pos = calloc(nthr,sizeof(int));
    if ( !pos ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    edge_t *min;
    int min_pos;
    unsigned int touched = 0;
    int leftmost = 0;

    do {

        min = &(edge_array[leftmost][pos[leftmost]]);
        min_pos = leftmost;

        for ( k = leftmost+1; k < nthr; k++ ) {
            if ( edge_array[k][pos[k]].weight != -1.0 &&
                 edge_array[k][pos[k]].weight < min->weight ) {
                min = &(edge_array[k][pos[k]]);
                min_pos = k;
            }
        }
        touched++;
        //printf("touched #%u\n", touched);
        //printf("%f, position %d within array %d\n", min->weight, pos[min_pos], min_pos);
        pos[min_pos] = pos[min_pos]+1;
        // if end of leftmost array reached
        while ( leftmost < nthr && edge_array[leftmost][pos[leftmost]].weight == -1.0 )
            leftmost++;
            
        // get rid of the empty array!
        //if ( edge_array[min_pos][pos[min_pos]].weight == -1.0 ) {
        //    free(edge_array[min_pos]);
        //    for ( j = min_pos+1; j < nlsts; j++ ) {
        //        edge_array[j-1] = edge_array[j];
        //        pos[j-1] = pos[j];
        //    }
        //    nlsts -= 1;
        //}

        //assert(heap_validate_heap_property(hp));
    } while ( touched < nthr*nedges );

    assert( touched == nthr*nedges );

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stderr, "findmin traverse       cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );

    for ( k = 0; k < nthr; k++ )
        free(edge_array[k]);
    free(edge_array);

    free(pos);

#endif

    return 0;
}
