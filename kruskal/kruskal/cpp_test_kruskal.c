/**
 * @file
 * Kruskal driver program
 */

#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "graph/graph.h"
//#include "graph/adjlist.h"
#include "graph/edgelist.h"
#include "kruskal.h"

#include "cpp_functions/cpp_psort.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
#endif


//#ifdef __cplusplus
//extern "C" {
//#endif

// C++ function declarations
//extern std::vector v;
///*edge_t * */void cpp_sort_edge_arr(edge_t *, size_t, int);
//int cpp_partition_edge_arr(edge_t *, size_t, int);
	
//void clear_vector();
//#ifdef __cplusplus
//	 }
//
//#endif

//extern void cpp_sort_el(edgelist_t *el);
//extern std::vector v;

int is_sorted(edge_t * edge_array, int from, int n)
{
    int e;
    edge_t *e1 = &edge_array[from];
    for ( e = from+1; e < from+n; e++ ) {
        if ( edge_compare(e1, &edge_array[e]) > 0 )
            return 0;
        e1++;
    }

    return 1;
}

int main(int argc, char **argv) 
{
    unsigned int *edge_membership, 
                 e,
                 is_undirected, 
                 msf_edge_count = 0;
    int next_option, print_flag;
    char graphfile[256];
    //adjlist_stats_t stats;
    edgelist_t *el;
    //adjlist_t *al;
    forest_node_t **fnode_array;
    int num_threads = 1;

    if ( argc < 2 ) {
        printf("Usage: ./kruskal --graph <graphfile> [parallel_sort_num_threads=1]\n"
                "\t\t --print\n");
        exit(EXIT_FAILURE);
    }

    print_flag = 0;

    /* getopt stuff */
    const char* short_options = "g:pn:";
    const struct option long_options[]={
        {"graph", 1, NULL, 'g'},
        {"print", 0, NULL, 'p'},
        {"nthr", 1, NULL, 'n'},
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

            case 'n':
                num_threads = atoi(optarg);
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
    //adjlist_init_stats(&stats);
    is_undirected = 0; // <- was 1

#ifdef PROFILE
    tsctimer_t tim;
    timer_clear(&tim);
    timer_start(&tim);
#endif
    //al = adjlist_read(graphfile, &stats, is_undirected);
    el = edgelist_read(graphfile, is_undirected, 0);
#ifdef PROFILE
    timer_stop(&tim);
    double hz = timer_read_hz();
    fprintf(stdout, "edgelist_read     cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    //fprintf(stdout, "al->nedges=%u\n", al->nedges);
    //adjlist_print(al);

    //el = edgelist_read(graphfile,is_undirected,0);
    fprintf(stdout, "Read graph\n\n");

    // Create edge list from adjacency list
    //el = edgelist_create(al);
    //edgelist_print(el);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    cpp_nth_element_edge_arr(el->edge_array,el->nedges,el->nedges/2);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, " nth_element       cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    srand( time(NULL) );
    int pivot_index = rand() % el->nedges;
    weight_t pivot_value = el->edge_array[pivot_index].weight;

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    long int partition_point = cpp_partition_edge_arr(el->edge_array,
                                                 el->nedges,
                                                 pivot_index);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, " part edge_array   cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    //for ( e = 0; e < el->nedges; e++)
    //    edge_print(&(el->edge_array[e]));
    printf(" pivot_index = %d, value = %f, partition_point = %ld (%lf%%)\n", pivot_index, pivot_value, partition_point, (double)partition_point/el->nedges*100);
    
    kruskal_init(el, /*al,*/ &fnode_array, &edge_membership);
    //kruskal_sort_edges(el);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    /*el->edge_array =*/ cpp_sort_edge_arr(el->edge_array,el->nedges,num_threads);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, " sort edge_array   cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    //assert( is_sorted(el->edge_array, 0, el->nedges) );


#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

    kruskal(el, fnode_array, edge_membership);

#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, " kruskal           cycles:%lf seconds:%lf freq:%lf\n", 
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

 
    //clear_vector();
    kruskal_destroy(el/*al*/, fnode_array, edge_membership);
    edgelist_destroy(el);
    //free(el);
    //adjlist_destroy(al);

    return 0;
}
