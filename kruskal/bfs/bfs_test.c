#include "graph/graph.h"
#include "graph/adjlist.h"
#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int main(int argc, char **argv)
{
    adjlist_t *al;
    int is_undirected;
    char graphfile[256];
    adjlist_stats_t stats;

    if ( argc == 1 ) {
        printf("Usage: %s --graph <graphfile>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* getopt stuff */
    int next_option;
    const char* short_options = "g";
    const struct option long_options[]={
        {"graph", 1, NULL, 'g'}
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options,
                                  NULL);
        switch(next_option) {
            case 'g':
                sprintf(graphfile, "%s", optarg);
                break;

            case -1:    // Done with options
                break;

            default:    // Unexpected error
                exit(EXIT_FAILURE);
        }
    } while ( next_option != -1 );

    /* read graph/create adjacency list */
    adjlist_init_stats(&stats);
    is_undirected = 1;
    al = adjlist_read(graphfile, &stats, is_undirected);

    fprintf(stdout, "Read graph\n\n");

    /* BFS stuff */
    //enum color *vertex_color;
    //queue_t *vertex_queue;

    int root = 0; //could be anything, e.g. a random one

    bfs_init(al/*, &vertex_color, &vertex_queue*/);
    bfs(al, root/*, vertex_color, vertex_queue*/);

    // Print results :)

    // Clean-up!
    bfs_destroy(/*vertex_color, vertex_queue*/);

    adjlist_destroy(al);

    return 0;
}
