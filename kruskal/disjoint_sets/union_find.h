/**
 * @file
 * Disjoint sets type definitions and function declarations
 */ 

#ifndef UNION_FIND_H_
#define UNION_FIND_H_

/**
 * Represents a node in the set forest
 */ 
typedef struct forest_node_st {
    void* value; //!< node contents
    int rank; //!< upper bound on the height of the node
    struct forest_node_st *parent; //!< the parent of the node
} forest_node_t;

extern forest_node_t* make_set(void* value);
extern void destroy_set(forest_node_t *node);
extern void union_sets(forest_node_t *node1, forest_node_t *node2);
extern forest_node_t* find_set(forest_node_t *node);

#endif 

