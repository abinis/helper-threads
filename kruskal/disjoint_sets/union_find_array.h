/**
 * @file
 * Disjoint-set type definitions and functions declarations
 *
 * Array implementation; the disjoint-set forest is supposed
 * to be represented as an array, of length V (where V is the
 * number of vertices in the graph), and whose each element
 * is of the struct type `union_find_node_t'. So if p is our
 * array, then p[i].parent points to the location of i-th
 * vertex's parent (actually, the vertex that acts as the
 * "represantative" of the set which it is part of, could be
 * one's self), and p[i].rank stores the height of the tree,
 * useful for employing the union-by-rank heuristic.
 */

#ifndef UNION_FIND_ARRAY_H_
#define UNION_FIND_ARRAY_H_

/* TODO */

typedef struct union_find_node_st {
    //void* value; //!< node contents
    int parent; //!< position of the parent node within the array
    int rank; //!< height of the node within the tree
} union_find_node_t;

extern union_find_node_t* union_find_array_init(int length);
extern void union_find_array_destroy(union_find_node_t* forest);
extern int union_find_array_find(union_find_node_t* arr, int index);
//extern int union_find_array_find(union_find_node_t* node);
extern void union_find_array_union(union_find_node_t* arr, int index1, int index2);
//extern void union_find_array_union(union_find_node_t* node1, union_find_node_t* node2);

#endif

