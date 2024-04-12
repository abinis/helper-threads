/**
 * @file
 * Disjoint-set type definitions and functions declarations
 *
 * Hash table implementation; the disjoint-set forest is re-
 * presented by a hash table consisting of m buckets. For a
 * given batch of e edges, the number of incident vertices v
 * is inside the range [(1+ceil(sqrt(1+8e)))/2, 2e], with the 
 * lower value corresponding to a complete graph with e edges, 
 * and the upper to e completely separate edges (i.e. no
 * common vertex between any two of them). Based on this
 * observation, we can derive a 'good' estimate for the number
 * of buckets m; we use TODO m = e/2 as a rule of thumb.
 *
 * Each entry in the hash table holds the id of the vertex,
 * (a pointer to) its parent vertex, and its rank (to be used
 * by union-by-rank). We use a pointer to refer to the parent
 * vertex instead of its id, in order to avoid hashing the id
 * value and looking up the target vertex within the bucket
 * it hashes to every time we follow parent references until
 * we reach the set representative (root) -- and every time we 
 * perform path compression.
 *
 */

#ifndef UNION_FIND_HASH_H_
#define UNION_FIND_HASH_H_

typedef struct union_find_hash_node_st {
    int id;
    int rank;
    struct union_find_hash_node_st *parent;
} union_find_hash_node_t;

typedef struct nodelist_st {
    union_find_hash_node_t data;
    struct nodelist_st *next;
} union_find_hash_nodelist_t;

typedef struct bucket_st {
    //int size;
    int count;
    union_find_hash_nodelist_t *elems;
} union_find_hash_bucket_t;

typedef struct union_find_hash_st {
    //int (*hash_me)();
    int m; // number of buckets
    int total_elems;
    //int *bucket_size; // bucket_size[i]: (current) maximum number of elements bucket i can hold
    //int *bucket_elems; // bucket_elems[i]: element count inside bucket i
    //union_find_hash_node_t **buckets; // each bucket is an array of nodes (could be list)
    union_find_hash_bucket_t *buckets;
} union_find_hash_t;

union_find_hash_t * union_find_hash_init(int m/*, int (*hash_me)(int)*/ );
//int union_find_hash_insert_new(union_find_hash_t *forest, int id);
union_find_hash_node_t * union_find_hash_find(union_find_hash_t *forest, int id);
void union_find_hash_union(union_find_hash_t *forest,
                           union_find_hash_node_t *set1,
                           union_find_hash_node_t *set2);
void union_find_hash_destroy(union_find_hash_t *forest);

#endif

