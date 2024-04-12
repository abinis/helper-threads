#include <stdio.h>

int main(int argc, char *argv[])
{
    int a = 10;
    printf("%d\n", a + ( a < 42 ));

    return 0;
}
