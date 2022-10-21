#ifndef MT_KRUSKAL_H_
#define MT_KRUSKAL_H_

#include "graph/edgelist.h"

#define MSF_EDGE 100
#define CYCLE_EDGE_MAIN 1

enum { MAIN_THR = 1, HELPER_THR };

typedef struct {
    int id; // thread-id
    int type; // thread type (main / helper)
    int begin, end; // thread bounds
} targs_t;

void kruskal_helper_init(edgelist_t *el, 
                         char **edge_color_main, 
                         char **edge_color_helper);

void *kruskal_ht(void *args);

void kruskal_helper_destroy(char *edge_color_main, 
                            char *edge_color_helper);

#endif
