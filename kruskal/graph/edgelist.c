/**
 * @file
 * Edge list function definitions
 */

#include "edgelist.h"

#include <stdio.h>
#include <stdlib.h>

#include "adjlist.h"

/**
 * Create an edge list from an adjacency list
 * @param al pointer to adjacency list array
 * @return pointer to created edge list
 */ 
edgelist_t* edgelist_create(adjlist_t *al)
{
    unsigned int v, edge_count = 0;
    node_t *w;
    edgelist_t *el = (edgelist_t*)malloc(sizeof(edgelist_t));
    if ( !el ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    el->nvertices = al->nvertices;
    el->nedges = al->is_undirected ? (al->nedges/2) : (al->nedges) ;
    el->is_undirected = al->is_undirected;
    el->edge_array = (edge_t*)malloc(el->nedges * sizeof(edge_t));
    if ( !el->edge_array ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    
    for ( v = 0; v < al->nvertices; v++ ) {
        for ( w = al->adj[v]; w != NULL; w = w->next ) {

            // Directed graph
            if ( !al->is_undirected ) {
                el->edge_array[edge_count].vertex1 = v; 
                el->edge_array[edge_count].vertex2 = w->id; 
                el->edge_array[edge_count].weight = w->weight;
                edge_count++;
            } 

            // Undirected graph: in this case, each edge is added twice in the 
            // adj. list representation, but we need it only once in the 
            // edge list representation. So, we add each edge (v,w) only 
            // when v<w
            else {
                if ( v < w->id ) {
                    el->edge_array[edge_count].vertex1 = v; 
                    el->edge_array[edge_count].vertex2 = w->id; 
                    el->edge_array[edge_count].weight = w->weight;
                    edge_count++;
                } else if ( v == w->id ) {
                    fprintf(stderr, "Something bad happened." 
                            "Self-edges should have been ignored. Exiting...\n");
                    exit(EXIT_FAILURE);
                }   
            }
        }
    }

    return el;
}

/**
 * Destroys the edge list
 * @param el pointer to edge list
 */ 
void edgelist_destroy(edgelist_t *el)
{
    free(el->edge_array);
    free(el);
}


/**
 * Prints the edge list
 * @param el pointer to the edge list
 */ 
void edgelist_print(edgelist_t *el)
{
    unsigned int i;

    for ( i = 0; i < el->nedges; i++ ) {
        fprintf(stdout, "Edge %u: (%u,%u) [%.2f]\n", 
                i, 
                el->edge_array[i].vertex1,
                el->edge_array[i].vertex2,
                el->edge_array[i].weight);
    }
}

/**
 * Edge comparison function for use with qsort
 * @param e1 first edge compared
 * @param e2 second edge compared
 * @return -1, 0, 1 if e1.weight <,=,> e2.weight, respectively
 */ 
inline int edge_compare(const void *e1, const void *e2)
{
    if ( ((edge_t*)e1)->weight < ((edge_t*)e2)->weight )
        return -1;
    else if ( ((edge_t*)e1)->weight > ((edge_t*)e2)->weight )
        return 1;
    else
        return 0;
}

