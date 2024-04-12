#include <stdio.h>
#include <stdlib.h>

#include "graph/adjlist.h"
#include "bfs.h"

enum color *vertex_color;

int head, tail;
int *vertex_queue;

node_t **heaviest_edge;

void bfs_init(adjlist_t *al/*, enum color **vertex_color, queue_t **vertex_queue*/)
{
    vertex_queue = calloc(al->nvertices, sizeof(int));
    if ( !vertex_queue ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    vertex_color = calloc(al->nvertices, sizeof(enum color));
    if ( !vertex_color ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    heaviest_edge = malloc((al->nvertices)*sizeof(node_t*));
    if ( !heaviest_edge ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
}

void bfs(adjlist_t *al, int root/*, enum color *vertex_color, queue_t *vertex_queue*/)
{
    int v, u;
    
    /* set all vertices to unexplored (A), their heaviest edge to NULL */
    for ( v = 0; v < al->nvertices; v++ ) {
        vertex_color[v] = 'A';
        heaviest_edge[v] = NULL;
    }

    head = 0;
    tail = 0;

    vertex_queue[tail++] = root;
    vertex_color[root] = 'Y';

    while ( head < tail ) {
        u = vertex_queue[head++];
        vertex_color[u] = 'E';

        node_t *x;
        for ( x = al->adj[u]; x != NULL; x = x->next ) {

            if ( vertex_color[x->id] == 'A' ) {
                vertex_queue[tail++] = x->id;
                vertex_color[x->id] = 'Y';
                if ( heaviest_edge[u] != NULL ) // all vertices u except root
                    heaviest_edge[x->id] = ( heaviest_edge[u]->weight > x->weight ) ? heaviest_edge[u] : x ;
                else // u is root
                    heaviest_edge[x->id] = x->weight;
            }
            /* a cycle-forming edge is detected */
            else {
                node_t max = ( heaviest_edge[u]->weight > x->weight ) ? heaviest_edge[u] : x;
                max = ( max->weight
            }

        }
    }
}

void bfs_destroy()
{
    free(vertex_queue);
    free(vertex_color);
    free(heaviest_edge);
}
