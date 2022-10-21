/**
 * @file
 * Adjacency list function definitions
 */

#include "adjlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util/util.h"

/**
 * Allocate and initialize adjacency list array
 * @param nvertices number of graph vertices
 * @return pointer to adjacency list
 */ 
adjlist_t* adjlist_init(unsigned int nvertices)
{
    unsigned int v;

    adjlist_t *al = (adjlist_t*)malloc(sizeof(adjlist_t));
    if ( !al ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    al->nvertices = nvertices;
    al->nedges = 0;
    al->adj = (node_t**)malloc(nvertices * sizeof(node_t*));
    if ( !al->adj ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    for ( v = 0; v < nvertices; v++ )
        al->adj[v] = NULL;

    return al;
}

/**
 * Inserts directed edge s->t to s' adjacency list
 * @param al pointer to adjacency list array
 * @param s edge source vertex id
 * @param t edge target vertex id
 * @param w edge weight
 * @param stats pointer to statistics info
 * @return 0 in case of success, 1 if edge already exists
 */ 
int adjlist_insert_edge(adjlist_t *al, 
                        unsigned int s, 
                        unsigned int t, 
                        weight_t w, 
                        adjlist_stats_t *stats)
{
    node_t *x; 
    for ( x = al->adj[s]; x != NULL; x = x->next ) {
        if ( x->id == t ) {
            stats->nparallel_edges++;
            return 1;
        }
    }

    x = (node_t*)malloc(sizeof(node_t));
    if ( !x ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    x->id = t;
    x->weight = w;
    x->next = al->adj[s];
    al->adj[s] = x;

    // @note for undirected graphs, edges will be counted twice 
    al->nedges++;
    stats->nedges++;

    return 0;
}


#define GRAPH_MAX_LINE_LENGTH 64

/**
 * Allocates and initializes an adjacency list from a graph file.
 * We assume that the graph in the file is represented in "coord" format:
 * c <comments>
 * c ....
 * p sp <nvertices> <nedges>  // general info
 * a <u1> <v1> <w1>   // arc1 info
 * a <u2> <v2> <w2>   // arc2 info
 * ...
 *
 * @param filename graph file name
 * @param stats pointer to statistics info
 * @param is_undirected undirected flag
 * @return pointer to adjacency list array
 */ 
adjlist_t* adjlist_read(const char *filename, 
                        adjlist_stats_t *stats, 
                        int is_undirected)
{
    unsigned int nvertices, nedges, s, t;
    weight_t weight;
    int fd;
    char *curr_line, dum1[2], dum2[2], a;

    curr_line = (char*)malloc(GRAPH_MAX_LINE_LENGTH * sizeof(char));
    if ( !curr_line ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    if ( (fd = open(filename, O_RDONLY)) < 0 ) {
        perror("Error while opening file from disk: ");
        exit(EXIT_FAILURE);
    }
    
    // Ignore comments until "p sp ... " line
    do {
        readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
    } while ( curr_line[0] != 'p' );

    sscanf(curr_line, "%s %s %u %u\n", dum1, dum2, &nvertices, &nedges);
    adjlist_t *al = adjlist_init(nvertices);
    al->is_undirected = is_undirected;
    stats->nvertices = nvertices;
    
    while ( readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH) > 0 ) {
        sscanf(curr_line, "%c %u %u %f\n", &a, &s, &t, &weight);
        
        // Self-edge test
        if ( s == t ) {
            fprintf(stderr, "Self-edge found (and ingored)! \n");
            stats->nloops++;
            continue;
        }

        // Add edge
        if ( !is_undirected ) 
            adjlist_insert_edge(al, s-1, t-1, weight, stats);

        // If graph is undirected, each edge is treated as bidirectional 
        // and therefore a second reverse edge is inserted 
        else {
            adjlist_insert_edge(al, s-1, t-1, weight, stats);
            adjlist_insert_edge(al, t-1, s-1, weight, stats);
        }
    }

    free(curr_line);

    return al;
}

/**
 * Prints the adjacency lists
 * @param al pointer to the adjacency list array
 */ 
void adjlist_print(adjlist_t *al)
{
    unsigned int v;
    node_t *w;

    for ( v = 0; v < al->nvertices; v++ ) {
        fprintf(stdout, "Node %u: ", v);
        for ( w = al->adj[v]; w != NULL; w = w->next ) {
            fprintf(stdout, "(%u->%u) [%.2f] ", v, w->id, w->weight);
        }
        fprintf(stdout, "\n");
    }
}

/**
 * Destroys the adjacency lists
 * @param al pointer to the adjacency list array
 */ 
void adjlist_destroy(adjlist_t *al)
{
    unsigned int v;
    node_t *w, *next;

    for ( v = 0; v < al->nvertices; v++ ) {
        w = al->adj[v];
        while ( w != NULL ) {
            next = w->next;
            free(w);
            w = next;
        }
    }
    free(al->adj);
    free(al);
}


/**
 * Initializes statistics info
 * @param stats pointer to statistics info
 */ 
void adjlist_init_stats(adjlist_stats_t *stats)
{
    stats->nvertices = 0;
    stats->nedges = 0;
    stats->nparallel_edges = 0;
    stats->nloops = 0;
    stats->avg_outdegree = 0.0;
    stats->max_outdegree = 0;
    stats->max_edge_weight = 0.0;
    stats->is_undirected = 0;
}

/**
 * Prints statistics
 * @param stats pointer to statistics info
 */ 
void adjlist_print_stats(adjlist_stats_t *stats)
{
    fprintf(stdout, "%s graph\n", 
            stats->is_undirected ? "Undirected" : "Directed"); 
    fprintf(stdout, "Vertices: %u\n", stats->nvertices);
    fprintf(stdout, "Edges: %u (%u if undirected)\n", 
            stats->nedges, stats->nedges/2);
    fprintf(stdout, "Parallel edges: %u\n", stats->nparallel_edges);
    fprintf(stdout, "Loops: %u\n", stats->nloops);
    fprintf(stdout, "Average out-degree: %f\n", stats->avg_outdegree);
    fprintf(stdout, "Maximum out-degree: %u\n", stats->max_outdegree);
    fprintf(stdout, "Maximum edge weight: %f\n", stats->max_edge_weight);
}

/**
 * Analyzes adjacency list and collects various statistics
 * @param al pointer to adjacency list array
 * @param stats pointer to statistics data structure
 */ 
void adjlist_analyze(adjlist_t *al, adjlist_stats_t *stats)
{
    int i;
    unsigned int v, out_degrees[10], curr_degree;
    node_t *w;

    for ( i = 0; i < 10; i++)
        out_degrees[i] = 0;

    for ( v = 0; v < al->nvertices; v++ ) {

        curr_degree=0;

        for ( w = al->adj[v]; w != NULL; w = w->next ) {
            stats->max_edge_weight = w->weight > stats->max_edge_weight 
                                     ? w->weight 
                                     : stats->max_edge_weight;
            curr_degree++;
        }

        stats->max_outdegree = curr_degree > stats->max_outdegree 
                               ? curr_degree 
                               : stats->max_outdegree; 

        stats->avg_outdegree = (stats->avg_outdegree + (float)curr_degree)/2;

        if ( curr_degree <= 8 )
            out_degrees[curr_degree]++;
        else
            out_degrees[9]++;
    }

    stats->is_undirected = al->is_undirected;
    fprintf(stdout, "\n");
    for ( i = 0; i < 9; i++ )
        fprintf(stdout, "#vertices with out-degree=%d: %u\n", 
                i, out_degrees[i]);
    fprintf(stdout, "#vertices with out-degree>=9: %u\n", 
            out_degrees[9]);

    fprintf(stdout, "\n\n");

}

