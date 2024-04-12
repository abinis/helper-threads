#ifndef MT_KRUSKAL_BITMASK_H_
#define MT_KRUSKAL_BITMASK_H_

#include "graph/edgelist.h"
#include "edge_color_bitmask.h"

/* Don't need these!
#define MSF_EDGE 100
#define CYCLE_EDGE_MAIN 1
*/

#define TARGS_LOOP_ARR_SZ 1024

enum { MAIN_THR = 1, HELPER_THR };

// TODO: multiple break reasons?
enum ht_break_reason { MAIN_FIN = 0, CYCLE_FOUND_BY_MAIN };

typedef struct {
    int id; // thread-id
    int type; // thread type (main / helper)
    int begin, end; // thread bounds
    int ht_loop_count; // how many times ht has gone through its edges
    char ht_cycles_per_loop[TARGS_LOOP_ARR_SZ]; // 'scratchpad',  
                             //stores cycles found by ht per loop
    //TODO redundant?
    char *p; // where inside the above to write
    enum ht_break_reason why; // why did the ht break its loop (e.g. main
                              // thread caught up to it, main finished etc
} targs_t;

unsigned int kruskal_helper_get_total_cycles(void *targs);

void kruskal_helper_print_stats(void *targs);

// No edge_color_helper in this implementation!

void kruskal_helper_init(edgelist_t *el, 
                         bitmask_t **edge_color_main);

void *kruskal_ht(void *args);

void kruskal_helper_destroy(bitmask_t *edge_color_main);

#endif
