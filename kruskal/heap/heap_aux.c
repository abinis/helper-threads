#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "heap.h"

/*
static int heap_edge_compare(const void *e1, const void *e2)
{
    if ( ((edge_t*)e1)->weight < ((edge_t*)e2)->weight )
        return -1;
    else if ( ((edge_t*)e1)->weight > ((edge_t*)e2)->weight )
        return 1;
    else
        return 0;
}
*/

static inline void swap(void **a, void **b)
{
    void *temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

static void heap_combine (heap_t *hp /*void edge_t **heap*/, size_t hs, int i)
{
    int l = 2*i+1;
    int r = 2*i+2;
    int mp = i;
    void **heap = hp->heap;
    if ( ( l < hs ) && ( hp->comp_func(heap[l],heap[mp]) < 0 ) )
        mp = l;
    if ( ( r < hs ) && ( hp->comp_func(heap[r],heap[mp]) < 0 ) )
        mp = r;
    if ( mp != i ) {
        swap((void**)&heap[i], (void**)&heap[mp]);
        heap_combine(hp, hs, mp);
    }
}

inline int heap_is_empty (heap_t *hp)
{
    return ( hp->hs <= 0 );
}

inline void /*edge_t*/ * heap_delete_min (heap_t *hp)
{
    assert(hp);

    if ( hp->hs <= 0 )
        return NULL;

    void /*edge_t*/ *min = hp->heap[0];
    hp->heap[0] = hp->heap[hp->hs-1];
    (hp->hs)--;
    heap_combine(hp/*->heap*/, hp->hs, 0);

    return min;
}

inline void heap_insert(heap_t *hp, void /*edge_t*/ *key)
{
    assert(hp);

    //printf("heap_insert:\n");

    /* We assume that the maximum heap size is known
     * and fixed (provided at heap creation with a call
     * to either heap_init or heap_create_empty),
     * and we'll never call heap_insert on an already
     * "full" heap! :)
     */
    (hp->hs)++;

    //printf("hs = %lu\n", hp->hs);

    hp->heap[hp->hs-1] = key;
    int i = hp->hs-1;
    int p = (i-1)/2;

    //printf("i = %d, p = %d, hs = %lu\n", i, p, hp->hs);
    //heap_print(hp);

    while ( ( i > 0 ) && ( hp->comp_func(hp->heap[p],hp->heap[i]) > 0 ) ) {
        swap((void**)&(hp->heap[p]), (void**)&(hp->heap[i]));
        //heap_print(hp);
        //printf("huh?\n");
        i = p;
        p = (i-1)/2;
        
        //printf("i = %d, p = %d\n", i, p);

    }
}

void /*edge_t*/ * heap_peek_min(heap_t *hp)
{
    assert(hp);

    if ( hp->hs <= 0 )
        return NULL;

    return hp->heap[0];
}

static void heap_construct(heap_t *hp)
{
    assert(hp);

    int i;
    for ( i = (hp->hs)/2; i >= 0; i--)
        heap_combine(hp/*->heap*/, hp->hs, i);
}

heap_t * heap_init(void /*edge_t*/ *array, 
                   size_t elem_sz, 
                   size_t nelem,
                   int (*comp_func)(const void *, const void *))
{
    heap_t *ret;
    
    ret = (heap_t*)malloc(sizeof(heap_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->heap = (void/*edge_t*/**)malloc(nelem*sizeof(void/*edge_t*/*));
    if ( !(ret->heap) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    // we cast our void* array to const char* array
    // in order to have byte addressability without
    // receiving void dereference compiler warnings :)
    const char *arr = (const char *)array;

    int i;
    for ( i = 0; i < nelem; i++ )
        ret->heap[i] = (void**)&(arr[i*elem_sz]);

    ret->hs = nelem;

    ret->comp_func = comp_func;

    heap_construct(ret);

    return ret;
}

heap_t * heap_create_empty(size_t nelem, 
                           int (*comp_func)(const void *, const void *))
{
    heap_t *ret;
    
    ret = (heap_t*)malloc(sizeof(heap_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->heap = (void/*edge_t*/**)calloc(nelem,sizeof(void/*edge_t*/*));
    if ( !(ret->heap) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->hs = 0;

    ret->comp_func = comp_func;

    return ret;
}

void heap_print(heap_t *hp)
{
    assert(hp);

    //printf("heap_print\n");
    edge_t **cast_heap = (edge_t**)(hp->heap);

    int i;
    for ( i = 0; i < hp->hs; i++ )
        printf("%d (%u %u %f)\n", i,
                               ((edge_t*)(cast_heap[i]))->vertex1, 
                               ((edge_t*)(cast_heap[i]))->vertex2, 
                               ((edge_t*)(cast_heap[i]))->weight);

    //printf("printed!\n");
}

void heap_destroy(heap_t *hp)
{
    assert(hp);

    //int i;
    //for ( i = 0; i < hp->hs; i++ )
    //    free(hp->heap[i]);

    free(hp->heap);
    free(hp);
}
