#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char *argv[])
{
    char mapping[3][4] = {{'c','p','t','\0'},
                        {'p','c','t','\0'},
                        {'t','c','p','\0'}};

    char mapping_s[3][4] = {"cpt",
                          "pct",
                          "tcp"};
    
    printf("%s %s %s\n", mapping[0], mapping[1], mapping[2]);
    printf("%s %s %s\n", mapping_s[0], mapping_s[1], mapping_s[2]);

    return 0;
}
