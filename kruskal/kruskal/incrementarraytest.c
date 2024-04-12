#include <stdio.h>

int main (int argc, char *argv[])
{
    int e[10] = {0,1,2,3,4,5,6,7,8,9};
    int t = 0;

    printf("%d ", e[t]);
    e[t]++;
    printf("%d \n", e[t]);

    return 0;
}
