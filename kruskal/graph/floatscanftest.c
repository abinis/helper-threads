#include <stdio.h>
//#include <stdlib.h>
#include <float.h>

int main(int argc, char *argv[])
{
    char line[256] = "a 6845 123757 1.72248e-5\n\0";

    char a;
    unsigned int s, t;
    float w;
    sscanf(line, "%c %u %u %f\n", &a, &s, &t, &w);

    printf("%c %u %u %f\n", a, s, t, w);

    printf("FLT_MAX=%1.6e\n", FLT_MAX);

    char firstline[256] = "1245\n\0";
    int ret = sscanf(firstline, "%u %u %f\n", &s, &t, &w);
    printf("ret=%d, s=%u, t=%u, w=%f\n", ret, s, t, w);

    return 0;
}
