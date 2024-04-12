#include <stdio.h>
#include <stdlib.h>

#include "doubly_linked_list.h"

int main (int argc, char *argv[])
{
    if ( argc < 3 ) {
        printf("usage: %s <heap_size> <nthreads>\n", argv[0]);
        exit(0);
    }

    doubly_linked_list_t *lp = doubly_linked_list_create();

    edge_t *edge_array;
    size_t nedges = atoi(argv[1]);
    int nthr = atoi(argv[2]);

    int i, j;
    int val1, val2;
    float w;

    srand(time(NULL));

    for ( i = 0; i < nthr; i++ ) {
        // we allocate +1 for the guard element :)
        edge_array = calloc(nedges+1,sizeof(edge_t));
        if ( !edge_array ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        for ( j = 0; j < nedges; j++ ) {
            val1 = rand() % 100;
            edge_array[j].vertex1 = val1;
            //printf("(%d ", val1);
            val2 = rand() % 100;
            while ( val2 == val1 ) val2 = rand() % 100;
            edge_array[j].vertex2 = val2;
            //printf("%d ", val2);
            w = (float) (rand() % 1000);
            edge_array[j].weight = w;
            //printf("%f)\n", w);
        }
        // finally, add the guard element :)
        // (we can't use -1 due to unsigned int)
        //edge_array[j].vertex1 = 0;
        //edge_array[j].vertex2 = 0;
        //edge_array[j].weight = -1.0;

        doubly_linked_list_insert(lp, edge_array);
        //doubly_linked_list_print(lp);
    }


    doubly_linked_list_destroy(lp);

    return 0;
}
