/**
 * Inserts directed edge s->t to s' adjacency list
 * @param al pointer to adjacency list array
 * @param s edge source vertex id
 * @param t edge target vertex id
 * @param w edge weight
 * @param stats pointer to statistics info
 * @return 0 in case of success (1 if edge already exists <- not any more!)
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
            // if edge already exists, just replace it with the
            // new one in case it is of smaller weight and return :)
            if ( x->weight > w ) {
                x->weight = w;
                return 0;
            }
            //return 1;
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

