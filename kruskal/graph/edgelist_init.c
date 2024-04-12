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

