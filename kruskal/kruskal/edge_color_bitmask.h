/**
 * @file
 * edge color storage structure bitmask implementation;
 * in the original implementation, edge color is stored in a
 * char array
 */

#ifndef EDGE_COLOR_BITMASK_H_
#define EDGE_COLOR_BITMASK_H_

#define BITS_PER_EDGE 2

typedef struct {
    unsigned int num_cells;
    unsigned int cell_size;
    unsigned int length;
    unsigned char *bitmask;
} bitmask_t;

// Probably an 'inside' function, no need to be in the .h file 
//unsigned int bitmask_get_cell_size();
bitmask_t * edge_color_bitmask_init(unsigned int nedges);
// new, 2-bit interface
int edge_color_bitmask_msf_edge(bitmask_t *bmp, unsigned int pos);
int edge_color_bitmask_cycle_main(bitmask_t *bmp, unsigned int pos);
int edge_color_bitmask_cycle_helper(bitmask_t *bmp, unsigned int pos);
// unmofied
int edge_color_bitmask_set_color(bitmask_t *bmp, unsigned int pos);
int edge_color_bitmask_set_color_atomic(bitmask_t *bmp, unsigned int pos);
int edge_color_bitmask_clear_color(bitmask_t *bmp, unsigned int pos);
// modified!
int edge_color_bitmask_get_color(bitmask_t *bmp, unsigned int pos);
void edge_color_bitmask_print(bitmask_t *bmp);
void edge_color_bitmask_destroy(bitmask_t *bmp);

#endif
