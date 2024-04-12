/**
 * @file
 * Edge list function definitions
 */

#include "edgelist.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "adjlist.h"
#include "util/util.h"

// Just to get rid of valgrind's 
// "conditional jump or move depends on uninitialised value(s)"
// error when weights are not available and -nan is printed.
// Can also test with f != f (true only if f is nan); this 
// doesn't seem to work without valgrind outputting errors
// (maybe has something to do with some compiler optimisation).
// Also, use isnan() function of math.h (link with -lm).
int weight_available = 0;

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

    memset(el,0,sizeof(edgelist_t));

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
                    fprintf(stderr, "Something bad happened. " 
                            "Self-edges should have been ignored. Exiting...\n");
                    exit(EXIT_FAILURE);
                }   
            }
        }
    }

    return el;
}

#ifndef GRAPH_MAX_LINE_LENGTH
#define GRAPH_MAX_LINE_LENGTH 256
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

    memset(el,0,sizeof(edgelist_t));
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

edgelist_w_id_t* edgelist_w_id_init(unsigned int nvertices, size_t nedges)
{
    edgelist_w_id_t *el;
    

    el = (edgelist_w_id_t*)malloc(sizeof(edgelist_w_id_t));
    if ( !el ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    memset(el,0,sizeof(edgelist_t));
    el->nvertices = nvertices; // We don't know yet! 
                               // We'll probably (?) find out tho
                               // Ok it's provided as an argument for now :)
    el->nedges = nedges;
    el->is_undirected = 1; // Not used for the time being
    el->edge_array = (edge_w_id_t*)malloc(el->nedges * sizeof(edge_w_id_t));
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

    //printf("%s orig_size=%u\n", __FUNCTION__, el->nedges);

    edge_t *edge_array_resized;

    edge_array_resized = realloc(el->edge_array, new_size * sizeof(edge_t));
    if ( !edge_array_resized ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    el->edge_array = edge_array_resized;
    el->nedges = new_size;

    //printf("%s new_size=%u\n", __FUNCTION__, el->nedges);

}

void edgelist_w_id_resize(edgelist_w_id_t* el, size_t new_size) 
{
    //size_t cur_num_edges = el->nedges;
    
    //cur_num_edges *= 2;

    edge_w_id_t *edge_array_resized;

    edge_array_resized = realloc(el->edge_array, new_size * sizeof(edge_w_id_t));
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
    char *curr_line, dum1[2], dum2[2], a[1];

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
    char *first_tok;
    do {
        readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
        //printf("curr_line[0] != 'p' ? %c\n", curr_line[0] != 'p' ? 'y' : 'n');
        //printf("first char: %c, equals 'a' ? %d\n", curr_line[0], curr_line[0] != 'a');
        char line_copy[GRAPH_MAX_LINE_LENGTH+1];
        memset(line_copy, 0, GRAPH_MAX_LINE_LENGTH+1);
        strncpy(line_copy,curr_line,GRAPH_MAX_LINE_LENGTH);
        //printf("line_copy=%s\n", line_copy);
        first_tok = strtok(line_copy," ");
        //printf("first_tok=%s\n", first_tok);
    //} while ( !isdigit(curr_line[0]) && curr_line[0] != 'a' );
    } while ( curr_line[0] != 'p' 
               && !isdigit(curr_line[0])
               && ( first_tok != NULL && 
                      !( strncmp(first_tok,"sp",2) == 0 
                         || strncmp(first_tok,"#sp",3) == 0 ) ) );

    //printf("curr_line=%s\n", curr_line);
    int file_format;
    // Apparently there is an old format (in folder /s/graphs/1K/random-old/)
    // where the first line consists of a single number, the number of vertices,
    // and the following lines contain a single edge each, as in format 1 below
    // (i.e. vertex1 vertex2 weight) :P
    if ( isdigit(curr_line[0]) ) {
        //file_format = 1;
        //int conv = sscanf(curr_line, "%u %u %f\n", &s, &t, &weight);
        sscanf(curr_line, "%u\n", &nvertices);
        // that's the case described above 
        //if ( conv == 1 )
        //    nvertices = s;
        // but it could also be that the file begins with an edge from the get-go!
        // (i.e. when it's a chunk produced by split, as in /s/graphs/bsp :) )
        // -> so we do treat that first line as an edge :D
        //else {
        //    file_format = 1;
        //    goto addedge;
        //}
    }
    else if ( first_tok != NULL && 
                ( strncmp(first_tok,"sp",2) == 0
                  || strncmp(first_tok,"#sp",3) == 0 ) ) {
        sscanf(curr_line, "%s %u %u\n", dum1, &nvertices, &nedges);
    }
    //printf("first lines ignored!");
    else {
        //printf("format: p sp V E\n");
        sscanf(curr_line, "%s %s %u %u\n", dum1, dum2, &nvertices, &nedges);
    }

    //printf("%u %u\n", nvertices, nedges);

    // We are aware of 2 input file formats:
    // 1) 0 : edge lines begin with 'a' (for 'arc') 
    // 2) 1 : edge lines begin with source vertex number :)
    readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
    file_format = ( curr_line[0] == 'a' ) ? 0 : 1 ;

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
            ret = sscanf(curr_line, "%s %u %u %f\n", a, &s, &t, &weight);
            if ( ret >= 4 )
                weight_available = 1;
            else if ( ret == 3 )
                weight = 0.0;
            // for an empty line, sscanf reads %c = '\n' into a :)
            // and ret = 1 (at least it should, but it doesn't seem
            // consistent :/ -- update: it's not, because readline
            // strips the trailing newline character!)
            else //if ( ret < 3 )
                continue;
        }
        else if ( file_format == 1 ) {
            ret = sscanf(curr_line, "%u %u %f\n", &s, &t, &weight);
            if ( ret >= 3 )
                weight_available = 1;
            else if ( ret == 2 )
                weight = 0.0;
            else //if ( ret < 2 )
                continue;
        }
        else {
            fprintf(stderr, "Something bad happened. " 
                    "Input file should have lines of edges beginning either with 'a' or with the source vertex number :/ Exiting...\n");
            exit(EXIT_FAILURE);
        }    

        // Empty line! triggered for file_format = 0,
        // sscanf reads %c = '\n' into a :) (it actually doesn't,
        // because readline strips the trailing newline char ;)
        //if ( ret == 1 ) {
        //    printf("empty line found! ignoring it...\n");
        //    continue;
        //}
        
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

        //addedge:
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


edgelist_w_id_t* edgelist_w_id_read(const char *filename,
                                    int is_undirected,
                                    int is_vertex_numbering_zero_based)
{ 
    edgelist_w_id_t *el;
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
    el = edgelist_w_id_init(nvertices, EDGELIST_NUM_EDGES_INIT);
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
            fprintf(stderr, "Self-edge found (and ignored)! \n");
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
        el->edge_array[edge_count].id = edge_count;

        edge_count++;


        // Resize edge_array if necessary
        if ( edge_count >= el->nedges ) {
            //printf("size before = %u\n", el->nedges);
            edgelist_w_id_resize(el, 2*(el->nedges));
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
    edgelist_w_id_resize(el, edge_count);

    free(curr_line);

    close(fd);

    return el;
}

static inline ssize_t do_io_bytes(int fd,
                                  void *buf,
                                  ssize_t bytes,
                                  int mode)
{
    ssize_t total_bytes = 0;
    ssize_t b;
    ssize_t bytes_rem = bytes;

    ssize_t (*io_oper)();
    if ( mode == 0 )
       io_oper = read;
    else if ( mode == 1 )
       io_oper = write;
    else {
       fprintf(stderr,"%s: unknown mode = %d :(\n", __FUNCTION__, mode);
       return -1;
    }

    do {
        again:
        if ( (b = (*io_oper)(fd, buf, bytes_rem)) < 0 ) {
            if ( errno == EINTR )
                goto again;
            return -1;
        } else if ( b == 0 ) // EOF
            break;
            //return 0;

        bytes_rem -= b;
        total_bytes += b;
    } while ( bytes_rem > 0 );

    return total_bytes;
}

int edgelist_dump(edgelist_t *el, const char *dumpfile)
{
    int fd;

    if ( (fd = open(dumpfile,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR)) < 0 ) {
        perror("Error while creating file: ");
        //exit(EXIT_FAILURE);
        return -1;
    }

    //ssize_t total_bytes_written = 0;
    ssize_t wb;
    // First, write the edgelist_st struct to the file
    ssize_t bytes_to_write = sizeof(edgelist_t);
    if ( (wb = do_io_bytes(fd,(void*)el,bytes_to_write,1)) < 0 ) {
        perror("Error while writing to file: ");
        //exit(EXIT_FAILURE);
        return -1;
    }
    //total_bytes_written += wb;
    if ( wb != bytes_to_write ) {
        fprintf(stderr,"Something unexpected happened... do_io_bytes() w/ write mode\n"
                       "should have insisted until the requested number of bytes was\n"
                       "written :/\n");
        return -1;
    }
    // Second, the edge_array :)
    bytes_to_write = el->nedges*sizeof(edge_t);
    if ( (wb = do_io_bytes(fd,(void*)(el->edge_array),bytes_to_write,1)) < 0 ) {
        perror("Error while writing to file: ");
        //exit(EXIT_FAILURE);
        return -1;
    }
    //total_bytes_written += wb;
    if ( wb != bytes_to_write ) {
        fprintf(stderr,"Something unexpected happened... do_io_bytes() w/ write mode\n"
                       "should have insisted until the requested number of bytes was\n"
                       "written :/\n");
        return -1;
    }

    fsync(fd);

    if ( close(fd) < 0 ) {
        perror("Error while closing file: ");
        //exit(EXIT_FAILURE);
        return -1;
    }

    //return total_bytes_written;
    return 1;
}

edgelist_t * edgelist_load(const char *dumpfile)
{
    int fd;

    if ( (fd = open(dumpfile,O_RDONLY)) < 0 ) {
        perror("Error while opening file from disk: ");
        exit(EXIT_FAILURE);
        //return NULL;
    }
 
    edgelist_t *el;

    // Allocate space for the edgelist_t struct :)
    el = (edgelist_t*)malloc(sizeof(edgelist_t));
    if ( !el ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
        //return NULL;
    }

    memset(el,0,sizeof(edgelist_t));

    ssize_t rb;
    // First, read the edgelist_t struct from the file
    ssize_t bytes_to_read = sizeof(edgelist_t);
    if ( (rb = do_io_bytes(fd,(void*)el,bytes_to_read,0)) < 0 ) {
        perror("Error while reading from file");
        exit(EXIT_FAILURE);
        //return -1;
    }
    if ( rb != bytes_to_read ) {
        fprintf(stderr,"Something unexpected happened... do_io_bytes() w/ read mode\n"
                       "should have insisted until the requested number of bytes\n"
                       "was read :/ -- EOF encountered earlier than expected\n");
        //return -1;
        exit(EXIT_FAILURE);
    }
    // Now we've read the edgelist_t struct, we know how much space we need
    // for the edge_array :)
    el->edge_array = (edge_t*)malloc(el->nedges*sizeof(edge_t));
    if ( !el->edge_array ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
        //return NULL;
    }

    // Second, read the edge_array :)
    bytes_to_read = el->nedges*sizeof(edge_t);
    if ( (rb = do_io_bytes(fd,(void*)(el->edge_array),bytes_to_read,0)) < 0 ) {
        perror("Error while reading from file");
        exit(EXIT_FAILURE);
        //return -1;
    }
    // taken when EOF is encountered before reading requested number of bytes
    if ( rb != bytes_to_read ) {
        fprintf(stderr,"Something unexpected happened... do_io_bytes() w/ read mode\n"
                       "should have insisted until the requested number of bytes\n"
                       "was read :/ -- EOF encountered earlier than expected\n");
        //return -1;
        exit(EXIT_FAILURE);
    }
    
    if ( close(fd) < 0 ) {
        perror("Error while closing file: ");
        exit(EXIT_FAILURE);
        //return -1;
    }

    return el;
}

edgelist_t * edgelist_choose_input_method(const char *filename,
                                          int is_undirected,
                                          int is_vertex_numbering_zero_based,
                                          char **method_chosen)
{
    edgelist_t *el;

    char *fullname = calloc(strlen(filename)+1,sizeof(char));
    if ( !fullname ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    // after the copy, the 'filename' string is already null-terminated
    // thanks to calloc
    strncpy(fullname,filename,strlen(filename));
    printf("fullname=%s\n", fullname);

    char *tok = strtok(fullname,"/");
    char *next_tok;
    while ( (next_tok = strtok(NULL,"/")) != NULL )
        tok = next_tok;
    //tok now holds the basename
    char *extension = strtok(tok,".");
    while ( (next_tok = strtok(NULL,".")) != NULL )
        extension = next_tok;

    printf("extension=%s\n", extension);

    *method_chosen = calloc(5,sizeof(char));
    if ( !(*method_chosen) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    strncpy(*method_chosen,"read",strlen("read"));
    if ( strcmp(extension,"dump") == 0 ) {
        strncpy(*method_chosen,"load",strlen("read"));
        printf("method_chosen=%s\n", *method_chosen);
        el = edgelist_load(filename);
    } else {
        printf("method_chosen=%s\n", *method_chosen);
        el = edgelist_read(filename,0,0);
    }

    free(fullname);

    return el;
}

static
edgelist_t* edgelist_parse_buffer(char *buffer, ssize_t bytes,
                                  int is_undirected,
                                  int is_vertex_numbering_zero_based)
{

    edgelist_t *el;
    unsigned int nvertices, nedges, s, t;
    weight_t weight;
    char dum1[2], dum2[2], a[1];

    unsigned int edge_count = 0;
    int skip_next_line = 0; // For undirected graphs, see below ;)
   

    char *curr_line /*= (char*)malloc(GRAPH_MAX_LINE_LENGTH * sizeof(char))*/;
    /*if ( !curr_line ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }*/
    
    // Ignore comments until "p sp ... " line
    // Edited: ignore lines up until the first one starting with a number
    //         or the first one starting with 'a' :)
    // Actually: we might just need the one starting with 'p' :P
    char *first_tok;
    char *saveptr_cur_line, *saveptr_first_tok;
    //printf("buffer contents: %s\n", buffer);
    char *str = buffer;
    do {
        //readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
        curr_line = strtok_r(str, "\n", &saveptr_cur_line);
        printf("first few lines: %s\n", curr_line);
        //printf("curr_line[0] != 'p' ? %c\n", curr_line[0] != 'p' ? 'y' : 'n');
        //printf("first char: %c, equals 'a' ? %d\n", curr_line[0], curr_line[0] != 'a');
        //char line_copy[GRAPH_MAX_LINE_LENGTH+1];
        //memset(line_copy, 0, GRAPH_MAX_LINE_LENGTH+1);
        //strncpy(line_copy,curr_line,GRAPH_MAX_LINE_LENGTH);
        //printf("line_copy=%s\n", line_copy);
        first_tok = strtok_r(curr_line," ", &saveptr_first_tok);
        //printf("first_tok=%s\n", first_tok);
        str = NULL;
    //} while ( !isdigit(curr_line[0]) && curr_line[0] != 'a' );
    } while ( curr_line[0] != 'p' 
               && !isdigit(curr_line[0])
               && ( first_tok != NULL && 
                      !( strncmp(first_tok,"sp",2) == 0 
                         || strncmp(first_tok,"#sp",3) == 0 ) ) );

    //printf("curr_line=%s\n", curr_line);
    int file_format;
    // Apparently there is an old format (in folder /s/graphs/1K/random-old/)
    // where the first line consists of a single number, the number of vertices,
    // and the following lines contain a single edge each, as in format 1 below
    // (i.e. vertex1 vertex2 weight) :P
    if ( isdigit(curr_line[0]) ) {
        //file_format = 1;
        //int conv = sscanf(curr_line, "%u %u %f\n", &s, &t, &weight);
        sscanf(curr_line, "%u\n", &nvertices);
        // that's the case described above 
        //if ( conv == 1 )
        //    nvertices = s;
        // but it could also be that the file begins with an edge from the get-go!
        // (i.e. when it's a chunk produced by split, as in /s/graphs/bsp :) )
        // -> so we do treat that first line as an edge :D
        //else {
        //    file_format = 1;
        //    goto addedge;
        //}
    }
    else if ( first_tok != NULL && 
                ( strncmp(first_tok,"sp",2) == 0
                  || strncmp(first_tok,"#sp",3) == 0 ) ) {
        sscanf(curr_line, "%s %u %u\n", dum1, &nvertices, &nedges);
    }
    //printf("first lines ignored!");
    else {
        //printf("format: p sp V E\n");
        sscanf(curr_line, "%s %s %u %u\n", dum1, dum2, &nvertices, &nedges);
    }

    //printf("%u %u\n", nvertices, nedges);

    // We are aware of 2 input file formats:
    // 1) 0 : edge lines begin with 'a' (for 'arc') 
    // 2) 1 : edge lines begin with source vertex number :)
    //readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH);
    curr_line = strtok_r(NULL, "\n", &saveptr_cur_line);
    file_format = ( curr_line[0] == 'a' ) ? 0 : 1 ;

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
            ret = sscanf(curr_line, "%s %u %u %f\n", a, &s, &t, &weight);
            if ( ret >= 4 )
                weight_available = 1;
            else if ( ret == 3 )
                weight = 0.0;
            // for an empty line, sscanf reads %c = '\n' into a :)
            // and ret = 1 (at least it should, but it doesn't seem
            // consistent :/ -- update: it's not, because readline
            // strips the trailing newline character!)
            else //if ( ret < 3 )
                continue;
        }
        else if ( file_format == 1 ) {
            ret = sscanf(curr_line, "%u %u %f\n", &s, &t, &weight);
            if ( ret >= 3 )
                weight_available = 1;
            else if ( ret == 2 )
                weight = 0.0;
            else //if ( ret < 2 )
                continue;
        }
        else {
            fprintf(stderr, "Something bad happened. " 
                    "Input file should have lines of edges beginning either with 'a' or with the source vertex number :/ Exiting...\n");
            exit(EXIT_FAILURE);
        }    

        // Empty line! triggered for file_format = 0,
        // sscanf reads %c = '\n' into a :) (it actually doesn't,
        // because readline strips the trailing newline char ;)
        //if ( ret == 1 ) {
        //    printf("empty line found! ignoring it...\n");
        //    continue;
        //}
        
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

        //addedge:
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

    //} while ( readline(fd, curr_line, GRAPH_MAX_LINE_LENGTH) > 0 );
    } while ( (curr_line = strtok_r(NULL, "\n", &saveptr_cur_line)) != NULL );

    // One last resize!
    //printf("edge_count = %u\n", edge_count);
    edgelist_resize(el, edge_count);

    //free(curr_line);

    return el;
}

/**
 *
 *
 */
edgelist_t* edgelist_single_read(const char *filename,
                            int is_undirected,
                            int is_vertex_numbering_zero_based)
{
    edgelist_t *el;

    // First, open the file...
    int fd;
    if ( (fd = open(filename, O_RDONLY)) < 0 ) {
        perror("Error while opening file from disk: ");
        exit(EXIT_FAILURE);
    }

    // ...then stat it...
    struct stat *statbuf = malloc(sizeof(struct stat));
    if ( !statbuf ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    int ret;
    if ( (ret = fstat(fd, statbuf)) != 0 ) {
        perror("Error while stating file: ");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_to_read = statbuf->st_size;

    // ...allocate a buffer large enough to hold all the contents...
    char *buffer;
    if ( !(buffer = calloc(bytes_to_read, sizeof(char))) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    
    // ...and fill it with a single read :)
    int rb;
    if ( (rb = do_io_bytes(fd,(void*)buffer,bytes_to_read,0)) < 0 ) {
        perror("Error while reading from file");
        exit(EXIT_FAILURE);
        //return -1;
    }
    if ( rb != bytes_to_read ) {
        fprintf(stderr,"Something unexpected happened... do_io_bytes() w/ read mode\n"
                       "should have insisted until the requested number of bytes\n"
                       "was read :/ -- EOF encountered earlier than expected\n");
        //return -1;
        exit(EXIT_FAILURE);
    }

    printf("do_io_bytes: read successful!\n");

    // Second, parse the contents of the buffer (raw bytes) into an edgelist!
    el = edgelist_parse_buffer(buffer,bytes_to_read,is_undirected,is_vertex_numbering_zero_based);

    free(statbuf);
    free(buffer);

    if ( close(fd) < 0 ) {
        perror("Error while closing file: ");
        exit(EXIT_FAILURE);
        //return -1;
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
        //int w = el->edge_array[i].weight;
        //int nan = ( w != w );
        fprintf(stdout, "Edge %u: (%u,%u) [%.2f]\n", 
                i, 
                el->edge_array[i].vertex1,
                el->edge_array[i].vertex2,
                el->edge_array[i].weight/*,
                ( weight_available ) ? el->edge_array[i].weight : 0.0*/ );
    }
}

void edgelist_w_id_print(edgelist_w_id_t *el)
{
    unsigned int i;

    for ( i = 0; i < el->nedges; i++ ) {
        //int w = el->edge_array[i].weight;
        //int nan = ( w != w );
        fprintf(stdout, "Edge %u: #%u (%u,%u) [%.2f]\n", 
                i, 
                el->edge_array[i].id,
                el->edge_array[i].vertex1,
                el->edge_array[i].vertex2,
                ( weight_available ) ? el->edge_array[i].weight : 0.0 );
    }
}

void edge_print(void /*edge_t*/ *edge)
{
    edge_t * ep = (edge_t*)edge;
    fprintf(stdout,"(%u,%u) [%.2f]\n", 
                ep->vertex1,
                ep->vertex2,
                /*( weight_available ) ?*/ ep->weight /*: 0.0 */); 
}

void edge_w_id_print(void /*edge_w_id_t*/ *edge)
{
    edge_w_id_t * ep = (edge_w_id_t*)edge;
    fprintf(stdout,"#%u (%u,%u) [%.2f]\n", 
                ep->id,
                ep->vertex1,
                ep->vertex2,
                ( weight_available ) ? ep->weight : 0.0 ); 
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

inline int edge_w_id_compare(const void *e1, const void *e2)
{
    if ( ((edge_w_id_t*)e1)->weight < ((edge_w_id_t*)e2)->weight )
        return -1;
    else if ( ((edge_w_id_t*)e1)->weight > ((edge_w_id_t*)e2)->weight )
        return 1;
    else
        return 0;
}

/**
 * Member-by-member comparison of two edge_t structs pointed to by
 * @param e1 and
 * @param e2
 * @return 1 if vertex1,vertex2 and weight members are all equal, 0 otherwise
 */
int edge_equality_check(edge_t *e1, edge_t *e2)
{
    if ( e1->vertex1 != e2->vertex1 )
        return 0;
    if ( e1->vertex2 != e2->vertex2 )
        return 0;
    if ( e1->weight != e2->weight )
        return 0;
    
    return 1;
}

