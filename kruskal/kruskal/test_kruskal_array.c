/**
 * @file
 * Kruskal driver program
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "graph/graph.h"
#include "graph/adjlist.h"
#include "kruskal_array.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
#endif

int main(int argc, char **argv) 
{
    unsigned int *edge_membership, 
                 e,
                 is_undirected, 
                 msf_edge_count = 0;
    int next_option, print_flag;
    char graphfile[256];
    adjlist_stats_t stats;
    edgelist_t *el;
    adjlist_t *al;
    union_find_node_t *fnode_array;

    //Little piece of code to print the "usage..." help
    //message neatly :P
    char whitesp_pad[32];
    snprintf(whitesp_pad, 32, "Usage: %s", argv[0]);
    int lastchar = ( strlen(whitesp_pad) > 32 ) ? 31 : strlen(whitesp_pad);
    whitesp_pad[lastchar] = '\0';
    int i;
    for ( i = 0; i < lastchar; i++ )
        whitesp_pad[i] = ' ';

    if ( argc == 1 ) {
        printf("Usage: %s --graph <graphfile>\n"
                "%s --print\n", argv[0], whitesp_pad);
        exit(EXIT_FAILURE);
    }

    print_flag = 0;

    /* getopt stuff */
    const char* short_options = "g:p";
    const struct option long_options[]={
        {"graph", 1, NULL, 'g'},
        {"print", 0, NULL, 'p'},
        {NULL, 0, NULL, 0}
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, 
                                  NULL);
        switch(next_option) {
            case 'p':
                print_flag = 1;
                break;

            case 'g':
                sprintf(graphfile, "%s", optarg);
                break;
            
            case '?':
                fprintf(stderr, "Unknown option!\n");
                exit(EXIT_FAILURE);

            case -1:    // Done with options
                break;  

            default:    // Unexpected error
                exit(EXIT_FAILURE);
        }

    } while ( next_option != -1 );

    // Init adjacency list
    adjlist_init_stats(&stats);
    is_undirected = 1;
    al = adjlist_read(graphfile, &stats, is_undirected);
    fprintf(stdout, "Read graph\n\n");

    // Create edge list from adjacency list
    el = edgelist_create(al);
    
    kruskal_init(el, al, &fnode_array, &edge_membership);
    kruskal_sort_edges(el);

#ifdef PROFILE
    tsctimer_t tim;
    timer_clear(&tim);
    timer_start(&tim);
#endif

    kruskal(el, fnode_array, edge_membership);

#ifdef PROFILE
    timer_stop(&tim);
    double hz = timer_read_hz();
    fprintf(stdout, "cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    weight_t msf_weight = 0.0;
    
    if ( print_flag )
        fprintf(stdout, "Edges in MSF:\n");

    for ( e = 0; e < el->nedges; e++ ) {
        if ( edge_membership[e] ) {
            msf_weight += el->edge_array[e].weight;
            msf_edge_count++;
            if ( print_flag ) {
                fprintf(stdout, "(%u,%u) [%.2f] \n", 
                        el->edge_array[e].vertex1,  
                        el->edge_array[e].vertex2,  
                        el->edge_array[e].weight);
            }
        }
    }

    fprintf(stdout, "Total MSF weight: %f\n", msf_weight);
    fprintf(stdout, "Total MSF edges: %d\n", msf_edge_count);

    kruskal_destroy(al, fnode_array, edge_membership);
    edgelist_destroy(el);
    adjlist_destroy(al);

    return 0;
}

