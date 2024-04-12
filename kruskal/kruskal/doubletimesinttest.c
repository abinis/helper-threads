#include <stdio.h>
#include <math.h>

int main (int argc, char *argv[])
{
    double perc = 0.991401;
    int scale = 60;

    printf("%d\n", (int)round(perc*scale));

    int i;
    for ( i = 0; i < (int)round(perc*scale); i++)
        printf(" ");
    printf("x\n");

    return 0;
}
