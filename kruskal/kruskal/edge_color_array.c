#include "edge_color_array.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

array_t * edge_color_array_init(unsigned int nedges)
{
    array_t *arrp = (array_t*)malloc(sizeof(array_t));
    if (!arrp) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    char *array = calloc(nedges, sizeof(char));
    if (!array) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    arrp->length = nedges;
    arrp->array = array;

    return arrp;
}

int edge_color_array_msf_edge(array_t *arrp, unsigned int pos)
{
    assert(arrp);

    if ( !(pos >= 0 && pos < arrp->length ) ) {
        fprintf(stderr, "function edge_color_array_msf_edge"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    arrp->array[pos] = 1;

    return 1;
}

int edge_color_array_cycle_main(array_t *arrp, unsigned int pos)
{
    assert(arrp);

    if ( !(pos >= 0 && pos < arrp->length ) ) {
        fprintf(stderr, "function edge_color_array_cycle_main"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    arrp->array[pos] = 2;

    return 1;
}

int edge_color_array_cycle_helper(array_t *arrp, unsigned int pos)
{
    assert(arrp);

    if ( !(pos >= 0 && pos < arrp->length ) ) {
        fprintf(stderr, "function edge_color_array_cycle_helper"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    arrp->array[pos] = 3;

    return 1;
}

int edge_color_array_get_color(array_t *arrp, unsigned int pos)
{
    assert(arrp);

    if ( !(pos >= 0 && pos < arrp->length ) ) {
        fprintf(stderr, "function edge_color_array_get_color:"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    return arrp->array[pos];
}

void edge_color_array_destroy(array_t *arrp)
{
    assert(arrp);

    free(arrp->array);
    free(arrp);
}
