/**
 * @file
 * Disjoint sets function definitions
 */ 

#include "union_find.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * Creates a set consisting of a single node
 * @param value pointer to node contents
 * @return pointer to the forest node created
 */
forest_node_t* make_set(int/*void**/ value) 
{
    forest_node_t* node = malloc(sizeof(forest_node_t));
    if ( !node ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    node->value = value;
    node->parent = NULL;
    node->rank = 0;
    
    return node;
}

/**
 * Deallocates memory of forest node
 * @param set pointer to forest node
 */ 
void destroy_set(forest_node_t *node)
{
    free(node);
}

/**
 * Unions two sets, employing the union-by-rank heuristic.
 * We make the root with smaller rank point to the root with larger rank.
 * @param node1 root of the first set
 * @param node2 root of the second set
 */
void union_sets(forest_node_t* node1, forest_node_t* node2) 
{
    if ( node1->rank > node2->rank ) {
        node2->parent = node1;
    } else if ( node2->rank > node1->rank ) {
        node1->parent = node2;
    } else { // equal
        node2->parent = node1;
        node1->rank++;
    }
}

/**
 * Finds the set of a node. Each set is uniquely identified by the root 
 * of its subtree. 
 * We apply the path-compression heuristic: we make each node on the 
 * find path point directly to the root.
 * @param node the node whose set we want to find
 * @return the root of the subtree where the node belongs to
 */ 
inline forest_node_t* find_set(forest_node_t* node) 
{
    forest_node_t* temp;

    // find the root of the tree
    forest_node_t* root = node;
    while ( root->parent != NULL ) {
        root = root->parent;
    }

    // compress the find path
    while ( node->parent != NULL ) {
        temp = node->parent;
        node->parent = root;
        node = temp;
    }

    return root;
}


