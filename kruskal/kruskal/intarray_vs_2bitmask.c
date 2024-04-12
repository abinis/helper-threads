/**
 * @file
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "edge_color_bitmask.h"
#include "machine/tsc_x86_64.h"

int main (int argc, char *argv[])
{
    int i;

    char *array;
    bitmask_t *bmp;

    if ( argc < 2 ) {
        printf("edge color char array vs bitmask comparison,\n"
               "<nedges> randomly marked edges\n"
               "    usage: %s <nedges>\n",
               argv[0]);
        exit(0);
    }

    long int nedges = atol(argv[1]);

    array = (char*)calloc(nedges, sizeof(char));
    bmp = edge_color_bitmask_init(nedges);

    srand(time(NULL));

    tsctimer_t tim;
    timer_clear(&tim);
    timer_start(&tim);

    for ( i = 0; i < nedges; i++ ) {
        int res = rand() % 1000;
        if (res < 500)
            array[i] = 1;
        else if (res < 800)
            array[i] = 2;
        else
            array[i] = 3;
    }

    timer_stop(&tim);
    double hz = timer_read_hz();
    fprintf(stdout, "array fill           cycles:%lf  freq:%.0lf seconds:%lf\n", 
                    timer_total(&tim), hz, timer_total(&tim)/hz);

    timer_clear(&tim);
    timer_start(&tim);

    for ( i = 0; i < nedges; i++ ) {
        int res = rand() % 1000;
        if (res < 500)
            edge_color_bitmask_msf_edge(bmp, i);
        else if (res < 800)
            edge_color_bitmask_cycle_main(bmp, i);
        else
            edge_color_bitmask_cycle_helper(bmp, i);
    }

    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "bitmask fill         cycles:%lf  freq:%.0lf seconds:%lf\n", 
                    timer_total(&tim), hz, timer_total(&tim)/hz);

    free(array);
    edge_color_bitmask_destroy(bmp);

    return 0;
}
