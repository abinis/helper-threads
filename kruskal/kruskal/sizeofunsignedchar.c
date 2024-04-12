#include <stdio.h>
#include <limits.h>

int main (int argc, char *argv[])
{
    //unsigned char c;

    printf("sizeof unsigned char = %lu\n", sizeof(unsigned char));
    printf("sizeof long int      = %lu\n", sizeof(long int));
    printf("  max  long int      = %lu\n", LONG_MAX);

    return 0;
}
