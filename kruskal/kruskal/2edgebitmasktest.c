#include <stdio.h>
#include "edge_color_bitmask.h"

int main (int argc, char *argv[])
{
    bitmask_t * bm = edge_color_bitmask_init(21);
    edge_color_bitmask_print(bm);

    int ret = edge_color_bitmask_msf_edge(bm, 0);
    printf("ret = %d, should be ok\n", ret);
    edge_color_bitmask_print(bm);

    edge_color_bitmask_cycle_helper(bm, 1);
    edge_color_bitmask_print(bm);
    edge_color_bitmask_cycle_helper(bm, 2);
    edge_color_bitmask_print(bm);
    edge_color_bitmask_cycle_helper(bm, 3);
    edge_color_bitmask_print(bm);
    edge_color_bitmask_cycle_helper(bm, 4);
    edge_color_bitmask_print(bm);
    edge_color_bitmask_cycle_helper(bm, 5);
    
    //edge_color_bitmask_msf_edge(bm, bm->length-1);

    edge_color_bitmask_print(bm);

    edge_color_bitmask_destroy(bm);

    return 0;
}
