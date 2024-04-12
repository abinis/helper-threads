int weight_available = 0;

#ifndef GRAPH_MAX_LINE_LENGTH
#define GRAPH_MAX_LINE_LENGTH 64
#endif

#define EDGELIST_NUM_EDGES_INIT 1024

edgelist_t* edgelist_init(unsigned int nvertices, size_t nedges)
{
    edgelist_t *el;
    

    el = (edgelist_t*)malloc(sizeof(edgelist_t));
    if ( !el ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    el->nvertices = nvertices; // We don't know yet! 
                               // We'll probably (?) find out tho
                               // Ok it's provided as an argument for now :)
    el->nedges = nedges;
    el->is_undirected = 1; // Not used for the time being
    el->edge_array = (edge_t*)malloc(el->nedges * sizeof(edge_t));
    if ( !el->edge_array ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    return el;
}

void edgelist_resize(edgelist_t* el, size_t new_size) 
{
    //size_t cur_num_edges = el->nedges;
    
    //cur_num_edges *= 2;

    edge_t *edge_array_resized;

    edge_array_resized = realloc(el->edge_array, new_size * sizeof(edge_t));
    if ( !edge_array_resized ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    el->edge_array = edge_array_resized;
    el->nedges = new_size;

}

edgelist_t* edgelist_read(const char *filename,
                          int is_undirected,
                          int is_vertex_numbering_zero_based)
{ 
    edgelist_t *el;
    unsigned int nvertices, nedges, s, t;
    weight_t weight;
    int fd;
    char *curr_line, dum1[2], dum2[2], a;

    unsigned int edge_count = 0;
    int skip_next_line = 0; // For undirected graphs, see below ;)
   

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
    // Edited: ignore lines up until the first one starting with a number
    //         or the first one starting with 'a' :)
    // Actually: we might just need the one starting with 'p' :P
    do {
        readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
        //printf("first char: %c, equals 'a' ? %d\n", curr_line[0], curr_line[0] != 'a');
    //} while ( !isdigit(curr_line[0]) && curr_line[0] != 'a' );
    } while ( curr_line[0] != 'p' );

    //printf("first lines ignored!");
    sscanf(curr_line, "%s %s %u %u\n", dum1, dum2, &nvertices, &nedges);

    // We are aware of 2 input file formats:
    // 1) 0 : edge lines begin with 'a' (for 'arc') 
    // 2) 1 : edge lines begin with source vertex number :)
    readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
    int file_format = ( curr_line[0] == 'a' ) ? 0 : 1 ;

    //sscanf(curr_line, "%s %s %u %u\n", dum1, dum2, &nvertices, &nedges);
    // We assume we don't know the number of edges (we ignored the
    // line starting with 'p')
    // Ok, for the time being, we actually read this line, but only
    // to get the number of vertices :P
    el = edgelist_init(nvertices, EDGELIST_NUM_EDGES_INIT);
    //adjlist_t *al = adjlist_init(nvertices);
    //al->is_undirected = is_undirected;
    //stats->nvertices = nvertices;
    
    do {
        // Assuming we have an undirected graph, with both
        // directions of each edge appearing in consecutive lines! :)
        // --> we read a line, skip the next, read, skip and so on ;)
        if ( skip_next_line) {
            skip_next_line = 0;
            continue;
        }
 
        int ret = 0;

        if ( file_format == 0 ) {
            ret = sscanf(curr_line, "%c %u %u %f\n", &a, &s, &t, &weight);
            if ( ret >= 4 )
                weight_available = 1;
        }
        else if ( file_format == 1 ) {
            ret = sscanf(curr_line, "%u %u %f\n", &s, &t, &weight);
            if ( ret >= 3 )
                weight_available = 1;
        }
        else {
            fprintf(stderr, "Something bad happened. " 
                    "Input file should have lines of edges beginning either with 'a' or with the source vertex number :/ Exiting...\n");
            exit(EXIT_FAILURE);
        }    
        
        // Self-edge test
        if ( s == t ) {
            fprintf(stderr, "Self-edge found (and ingored)! \n");
            //stats->nloops++;
            continue;
        }

        //printf("test for nan %d\n", weight != weight);
        /*int nan = ( weight != weight );
        if ( nan )
            printf("test for nan true!\n");
        else
            printf("test for nan false!\n");*/
        /*if ( weight_available )
            printf("ret=%d w=%f\n", ret, ( weight != weight ) ? 0.0 : weight);i*/

        // Add edge
        // ACTUALLY, this flag might be redundant, since, to the extent of our
        // knowledge, input files for undirected graphs, e.g. road graphs, 
        // explicitly provide the reverse edge (v->u) for every edge (u->v),
        // and, in fact, in *consecutive* lines, but ok :P
        //if ( !is_undirected ) 

        if ( !is_vertex_numbering_zero_based ) {
            s = s-1;
            t = t-1;
        }

        el->edge_array[edge_count].vertex1 = s;
        el->edge_array[edge_count].vertex2 = t;
        el->edge_array[edge_count].weight = weight;

        edge_count++;


        // Resize edge_array if necessary
        if ( edge_count >= el->nedges ) {
            //printf("size before = %u\n", el->nedges);
            edgelist_resize(el, 2*(el->nedges));
            //printf("size after = %u\n", el->nedges);
        }

        // If graph is undirected, each edge is treated as bidirectional 
        // and therefore a second reverse edge is inserted 
        // SEE ABOVE: we comment-out the following lines :)
        // else {
        //     adjlist_insert_edge(al, s-1, t-1, weight, stats);
        //     adjlist_insert_edge(al, t-1, s-1, weight, stats);
        // }

        if ( is_undirected )
            skip_next_line = 1;

    } while ( readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH) > 0 );

    // One last resize!
    //printf("edge_count = %u\n", edge_count);
    edgelist_resize(el, edge_count);

    free(curr_line);

    return el;
}

