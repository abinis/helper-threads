/**
 * @file
 * edge color array implementation (default is char array);
 * for comparison purposes with bitmask implemetation
 */

#ifndef EDGE_COLOR_ARRAY_H_
#define EDGE_COLOR_ARRAY_H_

typedef struct {
    unsigned int length;
    char * array;
} array_t;

array_t * edge_color_array_init(unsigned int nedges);
int edge_color_array_msf_edge(array_t *arrp, unsigned int pos);
int edge_color_array_cycle_main(array_t *arrp, unsigned int pos);
int edge_color_array_cycle_helper(array_t *arrp, unsigned int pos);
int edge_color_array_get_color(array_t *arrp, unsigned int pos);
//void edge_color_array_print(array_t arr);
void edge_color_array_destroy(array_t *arrp);

#endif
