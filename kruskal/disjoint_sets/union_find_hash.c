/* disjoint-set set data structure implementation
 * using an ancestor hash table (instead of tree nodes/trees
 * or a directly-addressable array)
 */

#include "union_find_hash.h"

#include <stdio.h>
#include <stdlib.h>

#define HASH_BUCKET_INIT_SIZE 4

union_find_hash_t * union_find_hash_init(int m/*, int (*hash_me)(int)*/ )
{
    union_find_hash_t *forest = malloc(sizeof(union_find_hash_t));
    if ( !forest ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    //union_find_hash_node_t **buckets = malloc(m*sizeof(union_find_hash_node_t*));
    //if ( !buckets ) {
    //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}

    union_find_hash_bucket_t *buckets = malloc(m*sizeof(union_find_hash_bucket_t));
    if ( !buckets ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    //int *bucket_size = malloc(m*sizeof(int));
    //if ( !bucket_size ) {
    //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}

    //int *bucket_elems = malloc(m*sizeof(int));
    //if ( !bucket_elems ) {
    //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
    //    exit(EXIT_FAILURE);
    //}

    // initialize buckets :)
    int i;
    for ( i = 0; i < m; i++ ) {
        // not needed for list implementation!
        //buckets[i].size = HASH_BUCKET_INIT_SIZE;
        buckets[i].count = 0;
        //if ( !buckets[i] ) {
        //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        //    exit(EXIT_FAILURE);
        //}
        //bucket_size[i] = 0;
        //bucket_elems[i] = 0;
        //buckets[i].elems = malloc(HASH_BUCKET_INIT_SIZE*sizeof(union_find_hash_node_t));
        //if ( !(buckets[i].elems) ) {
        //    fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        //    exit(EXIT_FAILURE);
        //}
        buckets[i].elems = NULL; //initially empty list :)
    }

    //forest->hash_me = hash_me;
    forest->m = m;
    forest->total_elems = 0;
    //forest->bucket_size = bucket_size;
    //forest->bucket_elems = bucket_elems;
    forest->buckets = buckets;

    return forest;
}

static inline
union_find_hash_node_t * union_find_hash_add_new(//union_find_hash_nodelist_t *where,
                                                 union_find_hash_t *forest,
                                                 union_find_hash_bucket_t *b,
                                                 int key)
{
    // Bad 
    //if ( b->count > b->size ) {
    //    fprintf(stderr,"%s: something bad happened :( count should be at most"
    //                   "equal to size\n", __FUNCTION__);
    //    //exit(EXIT_FAILURE);
    //    return NULL;
    //}
    // bucket resize needed!
    //if ( b->count == b->size ) {
    //    int new_size = 2*(b->size);
    //    printf("resizing bucket from %d to %d\n", b->size, new_size);
    //    union_find_hash_node_t *ret = realloc(b->elems,new_size*sizeof(union_find_hash_node_t));
    //    if ( !ret ) {
    //        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
    //        exit(EXIT_FAILURE);
    //    }
    //    b->elems = ret;
    //    b->size = new_size;
    //}

    //int pos = b->count;
    //b->elems[pos].id = key;
    //b->elems[pos].parent = &(b->elems[pos]);
    //b->elems[pos].rank = 0;

    //b->count = pos+1;

    union_find_hash_nodelist_t *new = malloc(sizeof(union_find_hash_nodelist_t));
    if ( !new ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    new->data.id = key;
    new->data.rank = 0;
    new->data.parent = &(new->data);
    // add it at the start of the bucket :)
    new->next = b->elems;
    b->elems = new;
    (b->count)++;
    // also update total element count
    (forest->total_elems)++;

    return &(new->data);
}

static inline
union_find_hash_node_t * union_find_hash_search(union_find_hash_t *forest, int key)
{
    //union_find_hash_node_t *p;

    int bucket = key % forest->m; // simple modulo hash function!
    //TODO:
    //int bucket = forest->hash_me(key);
    //int i;
    //for ( i = 0; i < forest->buckets[bucket].count; i++ ) {
    //    p = &(forest->buckets[bucket].elems[i]);
    //    if ( p->id == key )
    //        return p;
    //}
    union_find_hash_nodelist_t *p = forest->buckets[bucket].elems;
    //union_find_hash_nodelist_t *prev = p;
    while ( p != NULL ) {
        if ( p->data.id == key )
            return &(p->data);
        //prev = p;
        p = p->next;
    }

    // requested key not found :) add it...
    return union_find_hash_add_new(forest,&(forest->buckets[bucket]),key);
}

union_find_hash_node_t * union_find_hash_find(union_find_hash_t *forest, int id)
{
    union_find_hash_node_t *root = union_find_hash_search(forest,id);
    union_find_hash_node_t *temp = root;
    while ( root->parent != root )
       root = root->parent;

    // path compression :)
    union_find_hash_node_t *parent;
    while ( temp->parent != root ) {
        parent = temp->parent;
        temp->parent = root;
        temp = parent;
    }

    return root;
}

void union_find_hash_union(union_find_hash_t *forest,
                           union_find_hash_node_t *set1,
                           union_find_hash_node_t *set2)
{
    if ( set1->rank > set2->rank ) {
        set2->parent = set1;
    } else if ( set2->rank > set1->rank ) {
        set1->parent = set2;
    } else { //equal
        set2->parent = set1;
        (set1->rank)++;
    }
}

void union_find_hash_destroy(union_find_hash_t* forest)
{
    int i;
    for ( i = 0; i < forest->m; i++ ) {
        // free element list of each bucket :)
        union_find_hash_nodelist_t *p = forest->buckets[i].elems;
        union_find_hash_nodelist_t *prev = p;
        while ( p != NULL ) {
            p = p->next;
            free(prev);
            prev = p;
        }
    }
    free(forest->buckets);

    //free(bucket_size);
    //free(bucket_elems);

    free(forest);
}
