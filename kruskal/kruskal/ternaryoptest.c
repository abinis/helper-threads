#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void print_array( int *arr, int len )
{
    assert(arr);
    int i;
    printf("{");
    for ( i = 0; i < len; i++)
        printf("%2d%c", arr[i], (i < len-1)?',':'}');
    printf("\n");
}

int main (int argc, char *argv[])
{
    if ( argc < 4 ) {
        printf("usage: %s <begin> <end> <nthr>\n", argv[0]);
        exit(1);
    }

    int color[20] = {1,1,1,0,1,1,0,0,0,1,1,0,0,0,0,1,1,1,1,1};
    int edge[20] = {5,11,15,3,19,4,2,8,0,12,14,1,6,7,10,9,17,13,18,16};

    int len = 20;

    int begin = atoi(argv[1]);
    int end = atoi(argv[2]);
    int nthr = atoi(argv[3]);

    int t, from, to, i, e;

    printf("edge =\n");
    print_array( (int*)edge, len);

    printf("color =\n");
    print_array( (int*)color, len);

    assert(end >= begin);
    assert(nthr);
   
    int chunk_size = ( end - begin ) / nthr;
    int rem = ( end - begin ) % nthr;
    
    for ( t = 0; t < nthr; t++ ) {
        from = begin + (( t < rem ) ? t*(chunk_size+1) : rem*(chunk_size+1)+(t-rem)*chunk_size);
        to = from + (( t < rem ) ? chunk_size+1 : chunk_size);
        printf("t:%d [%d->%d)\n", t, from, to);
        e = from;
        for ( i = from; i < to; i++ ) {
            if ( color[i] == 1 ) {
                color[e] = 0;
                // also bring the actual edges together!
                edge[e++] = edge[i];
            }
        }
        // then, fill the last to-e positions with jumps :)
        // *all the small jumps should be pushed together into a
        // single jump streak at the end of the array, e.g.
        // |-1| 0| 0|-2|-1| 0| 0| 0|-1| --> | 0| 0| 0| 0| 0|-4|-3|-2|-1|
        i = e;
        while ( i < to )
        {
            color[i] = i - to;
            i++;
        }
    }

    printf("edge after =\n");
    print_array( (int*)edge, len);
    printf("color after =\n");
    print_array( (int*)color, len);

    return 0;
}
