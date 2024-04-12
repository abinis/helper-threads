#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[])
{
    unsigned int array[16] = {1,1,1,2,2,2,1,1,1,1,2,2,2,2,2,1};

    int i;
    unsigned int mask = 1U << 1;
    for ( i = 0; i < 16; i++ ) {
        array[i] &= mask;
        printf("%2d", array[i]);
    }
    printf("\n");
    
    int negid = -1;
    for ( i = 16-1; i >= 0; i-- ) {

        // if edge isn't marked as a cycle, skip it...
        if ( array[i] != 2 ) {
            // ...this also ends our streak
            negid = -1;
            continue;
        }

        array[i] = negid;
        negid--;
    }

    for ( i = 0; i < 16; i++ )
        printf("%2d", array[i]);
    printf("\n");

    return 0;
}
