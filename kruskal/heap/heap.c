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

static inline void swap_aug(void *a, void *b, size_t elem_sz)
{
    heap_node_t *hna = (heap_node_t*)a;
    heap_node_t *hnb = (heap_node_t*)b;
    heap_node_t temp;

    //edge_t *ae = (edge_t*)(hna->data);
    //edge_t *be = (edge_t*)(hnb->data);
    //printf("%s a->data: (%u %u %f)\n", __FUNCTION__, ae->vertex1,
    //                                        ae->vertex2,
    //                                        ae->weight);
    //printf("%s b->data: (%u %u %f)\n", __FUNCTION__, be->vertex1,
    //                                        be->vertex2,
    //                                        be->weight);

    temp = *hna;
    *hna = *hnb;
    *hnb = temp;

    //int byte;

    //char temp[elem_sz];
    //char *ac = (char*)a;
    //char *bc = (char*)b;

    //for ( byte = 0; byte < elem_sz; b++ ) {
    //    temp[byte] = ac[byte];
    //    ac[byte] = bc[byte];
    //    bc[byte] = temp[byte];
    //}
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

static void heap_aug_combine (heap_aug_t *hp, size_t hs, int i)
{
    int l = 2*i+1;
    int r = 2*i+2;
    int mp = i;
    heap_node_t *heap = hp->heap;
    if ( ( l < hs ) && ( hp->comp_func(heap[l].data,heap[mp].data) < 0 ) )
        mp = l;
    if ( ( r < hs ) && ( hp->comp_func(heap[r].data,heap[mp].data) < 0 ) )
        mp = r;
    if ( mp != i ) {
        //printf("%s hs=%lu i=%d\n", __FUNCTION__, hs, i);
        //edge_print(heap[i].data);
        //edge_print(heap[mp].data);
        swap_aug((void*)&heap[i], (void*)&heap[mp], sizeof(*heap));
        heap_aug_combine(hp, hs, mp);
    }
}

inline int heap_is_empty (heap_t *hp)
{
    return ( hp->hs <= 0 );
}

inline int heap_aug_is_empty (heap_aug_t *hp)
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

inline void /*edge_t*/ * heap_aug_delete_min (heap_aug_t *hp)
{
    assert(hp);

    if ( hp->hs <= 0 )
        return NULL;

    void /*edge_t*/ *min = hp->heap[0].data;
    swap_aug(&(hp->heap[0]),&(hp->heap[hp->hs-1]),sizeof(*(hp->heap)));
    (hp->hs)--;
    heap_aug_combine(hp/*->heap*/, hp->hs, 0);

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

inline void heap_aug_insert(heap_aug_t *hp, void /*edge_t*/ *key)
{
    assert(hp);

    //printf("heap_insert:\n");

    /* We assume that the maximum heap size is known
     * and fixed (provided at heap creation with a call
     * to either heap_init or heap_create_empty),
     * and we'll never call heap_insert on an already
     * "full" heap! :)
     */

    //printf("hs = %lu\n", hp->hs);
    heap_node_t *heap = hp->heap;
 
    //printf("%s next_addr = %p, compared against %p\n", __FUNCTION__, key,
    //                                              heap[hp->hs].maxv);

    if ( heap[hp->hs].maxv < key )
        return;

    (hp->hs)++;

    heap[hp->hs-1].data = key;
    int i = hp->hs-1;
    int p = (i-1)/2;

    //printf("i = %d, p = %d, hs = %lu\n", i, p, hp->hs);
    //heap_print(hp);

    while ( ( i > 0 ) && ( hp->comp_func(heap[p].data,heap[i].data) > 0 ) ) {
        swap_aug((void*)&heap[p], (void*)&heap[i], sizeof(*heap));
        //heap_print(hp);
        //printf("huh?\n");
        i = p;
        p = (i-1)/2;
        
        //printf("i = %d, p = %d\n", i, p);

    }
}

void heap_aug_increase_root_key(heap_aug_t *hp, void *new_key)
{
    assert(hp);

    heap_node_t *heap = hp->heap;

    if ( heap[0].maxv < new_key ) {
        heap_aug_delete_min(hp);
        return;
    }

    heap[0].data = new_key;
    heap_aug_combine(hp/*->heap*/, hp->hs, 0);
}

void /*edge_t*/ * heap_peek_min(heap_t *hp)
{
    assert(hp);

    if ( hp->hs <= 0 )
        return NULL;

    return hp->heap[0];
}

void * heap_aug_peek_min(heap_aug_t *hp)
{
    assert(hp);

    if ( hp->hs <= 0 )
        return NULL;

    return hp->heap[0].data;
}

static void heap_construct(heap_t *hp)
{
    assert(hp);

    int i;
    for ( i = (hp->hs)/2; i >= 0; i-- )
        heap_combine(hp/*->heap*/, hp->hs, i);
}

void heap_aug_construct(heap_aug_t *hp)
{
    assert(hp);

    int i;
    for ( i = (hp->hs)/2; i >= 0; i-- )
        heap_aug_combine(hp, hp->hs, i);
}

heap_t * heap_init(void /*edge_t*/ *array, 
                   size_t elem_sz, 
                   size_t nelem,
                   int (*comp_func)(const void *, const void *))
{
    assert(array);

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

heap_aug_t * heap_aug_init(void **data,
                           void **maxv,
                           size_t elemsz,
                           size_t nelem,
                           int (*comp_func)(const void *, const void *))
{
    assert(data);
    assert(maxv);

    heap_aug_t *ret;

    ret = (heap_aug_t*)malloc(sizeof(heap_aug_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->heap = (heap_node_t*)malloc(nelem*sizeof(heap_node_t));
    if ( !(ret->heap) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    // we cast our void* array to const char* array
    // in order to have byte addressability without
    // receiving void dereference compiler warnings :)
    //const char *data_char = (const char *)data;
    //const char *maxv_char = (const char *)maxv;

    int i;
    for ( i = 0; i < nelem; i++ ) {
        ret->heap[i].data = data[i]/*(void*)&(data_char[i*elemsz])*/;
        ret->heap[i].maxv = maxv[i]/*(void*)&(maxv_char[i*elemsz])*/;
        //printf("%s #%d data_addr = %p\n", __FUNCTION__, i, ret->heap[i].data);
        //printf("%s #%d maxv_addr = %p\n", __FUNCTION__, i, ret->heap[i].maxv);
    }

    //printf("%s data and maxv arrays parsed! :)\n", __FUNCTION__);

    ret->hs = nelem;

    //printf("%s nelem = %lu\n", __FUNCTION__, nelem);

    ret->comp_func = comp_func;

    heap_aug_construct(ret);

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

heap_aug_t * heap_aug_create_empty(size_t nelem, 
                                 int (*comp_func)(const void *, const void *))
{
    heap_aug_t *ret;
    
    ret = (heap_aug_t*)malloc(sizeof(heap_aug_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->heap = (heap_node_t*)malloc(nelem*sizeof(heap_node_t));
    if ( !(ret->heap) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->hs = 0;

    ret->comp_func = comp_func;

    return ret;
}

void heap_aug_add(heap_aug_t *hp, void *data, void *maxv)
{
    assert(hp);

    hp->heap[hp->hs].data = data;
    hp->heap[hp->hs].maxv = maxv;

    (hp->hs)++;
}

void heap_print(heap_t *hp, void (*print_func)(void *))
{
    assert(hp);

    //printf("heap_print\n");
    //edge_t **cast_heap = (edge_t**)(hp->heap);

    // we cast our void* array to const char* array
    // in order to have byte addressability without
    // receiving void dereference compiler warnings :)
    //const char *arr = (const char *)(hp->heap);

    int i;
    for ( i = 0; i < hp->hs; i++ )
        /*printf("%d (%u %u %f)\n", i,*/ print_func(hp->heap[i])/*)*/;
                               //((edge_t*)(cast_heap[i]))->vertex1, 
                               //((edge_t*)(cast_heap[i]))->vertex2, 
                               //((edge_t*)(cast_heap[i]))->weight);

    //printf("printed!\n");
}

void heap_aug_print(heap_aug_t *hp, void (*print_func)(void *))
{
    assert(hp);

    int i;
    for ( i = 0; i < hp->hs; i++ )
        print_func(hp->heap[i].data);
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

void heap_aug_destroy(heap_aug_t *hp)
{
    assert(hp);

    free(hp->heap);
    free(hp);
}
