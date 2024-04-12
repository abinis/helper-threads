#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "union_find_hash.h"
#include "graph/edgelist.h"
#include "util/util.h"

static inline
void * allocate_buffer(size_t bytes)
{
    void *buf;
    buf = malloc(bytes);
    if ( !buf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    return buf;
}

edgelist_t * read_graph_chunk(char *filename, int nedges)
{
    edgelist_t *ret;
    int fd;

    if ( (fd = open(filename, O_RDONLY)) < 0 ) {
        perror("Error while opening file from disk: ");
        exit(EXIT_FAILURE);
    }

    ret = malloc(sizeof(edgelist_t));
    if ( !ret ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    if ( !(ret->edge_array = malloc(nedges*sizeof(edge_t))) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    int is_undirected = 0;

    char *curr_line = (char*)allocate_buffer(256);
    //char *prev_line = (char*)allocate_buffer(256);
    int s, t, sp, tp;
    weight_t w;
    int count = 0;
    while ( readline(fd, curr_line, 256) > 0 ) {
        sscanf(curr_line, "%u %u %f\n", &s, &t, &w);
        // check for consecutive symmetric edges ;)
        if ( count > 0 && sp == t && tp == s ) {
            // we now know it's undirected
            is_undirected = 1;
            continue;
        }
        // add edge
        ret->edge_array[count].vertex1 = s;
        ret->edge_array[count].vertex2 = t;
        ret->edge_array[count].weight = w;
        count++;
        // store previous values 
        sp = s;
        tp = t;
    }

    // resize edge_array from nedges to count
    if ( !(ret->edge_array = realloc(ret->edge_array,count*sizeof(edge_t))) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    ret->nvertices = 0; // we don't know yet :/
    ret->nedges = count;
    ret->is_undirected = is_undirected;

    // cleanup!
    free(curr_line);
    if ( close(fd) < 0 ) {
        perror("Error while closing file: ");
        exit(EXIT_FAILURE);
    }

    return ret;
}

int main(int argc, char *argv[])
{
    if ( argc < 4 ) {
        printf("Usage: %s <split_file> <file_lines> <number_of_buckets>\n", argv[0]);
        exit(EXIT_FAILURE);
    }


    int nedges = atoi(argv[2]);
    int m = atoi(argv[3]);
    //int keys = atoi(argv[2]);

    union_find_hash_t *forest = union_find_hash_init(m);

    //int i;
    //for ( i = 0; i < keys; i++ ) {
    //    int key = rand() % 1000;
    //    union_find_hash_node_t * np = union_find_hash_find(forest,key);
    //    printf("%d->%d\n", key, np->id);
    //}

    // read graph file chunk, hash vertices as they show up :)
    edgelist_t *el = read_graph_chunk(argv[1],nedges);
    //edgelist_t *el = edgelist_choose_input_method(argv[1],is_undirected,0,&inp_method);

    //edgelist_print(el);

    qsort(el->edge_array, el->nedges, sizeof(edge_t), edge_compare);

    long msf_edges = 0;
    // TODO: correct size :P
    edge_t **msf = (edge_t**)malloc(el->nedges*sizeof(edge_t*));
    if ( !msf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    union_find_hash_node_t *set1, *set2;
    edge_t *pe;
    int i;
    for ( i = 0; i < el->nedges; i++ ) { 
        pe = &(el->edge_array[i]);
    
        set1 = union_find_hash_find(forest, pe->vertex1);
        set2 = union_find_hash_find(forest, pe->vertex2);

        // vertices belong to different forests
        if ( set1 != set2 ) {
            union_find_hash_union(forest, set1, set2);
            msf[msf_edges] = pe;
            msf_edges++;
            //edge_membership[i] = 1;
        } /*else
            edge_membership[i] = 2;*/
    }

    printf("vertices found = %d\n", forest->total_elems);

    //edgelist_print(el);
    free(msf);
    edgelist_destroy(el);
    union_find_hash_destroy(forest);

    return 0;
}
