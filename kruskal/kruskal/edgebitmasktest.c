#include <stdio.h>
#include "edge_color_bitmask.h"

int main (int argc, char *argv[])
{
    bitmask_t * bm = edge_color_bitmask_init(96);

    int ret = edge_color_bitmask_set_color(bm, 42);
    printf("ret = %d, should be ok\n", ret);

    ret = edge_color_bitmask_set_color(bm, 999);
    ret = edge_color_bitmask_set_color(bm, 1);
    ret = edge_color_bitmask_set_color(bm, 0);
    ret = edge_color_bitmask_set_color(bm, 96);
    ret = edge_color_bitmask_set_color(bm, 97);
    ret = edge_color_bitmask_set_color(bm, 98);
    ret = edge_color_bitmask_set_color(bm, 99);
    ret = edge_color_bitmask_set_color(bm, bm->length-1);
    ret = edge_color_bitmask_set_color(bm, 8);

    ret = edge_color_bitmask_set_color(bm, 16);
    printf("ret = %d, should be atomically set to 1\n", ret);

    printf("some bits: #0=%d #1=%d #8=%d #10=%d #42=%d #998=%d #999=%d\n", 
              edge_color_bitmask_get_color(bm, 0),
              edge_color_bitmask_get_color(bm, 1),
              edge_color_bitmask_get_color(bm, 8),
              edge_color_bitmask_get_color(bm, 10),
              edge_color_bitmask_get_color(bm, 42),
              edge_color_bitmask_get_color(bm, 998),
              edge_color_bitmask_get_color(bm, 999));

    int i;
    for ( i = 0; i < bm->num_cells; i++)
       printf("%d ", bm->bitmask[i]);

    printf("\n");
    edge_color_bitmask_print(bm);

    ret = edge_color_bitmask_clear_color(bm, 42);

    for ( i = 0; i < bm->num_cells; i++)
       printf("%d ", bm->bitmask[i]);

    printf("\n");
    edge_color_bitmask_print(bm);

    

    edge_color_bitmask_destroy(bm);

    return 0;
}
