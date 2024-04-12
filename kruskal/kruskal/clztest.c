#include <stdio.h>

int main (int argc, char *argv[])
{
    unsigned int v = 1U;

    int lz = __builtin_clz(v);

    unsigned int bucket;

    if ( v && !(v & (v - 1)) )
        bucket = v;
    else
        bucket = 1U << (32-lz);

    printf("%u\n%d\n%u\n", v, lz, bucket);

    return 0;
}
