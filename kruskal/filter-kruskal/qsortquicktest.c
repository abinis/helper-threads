#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int int_compare(const void *a, const void *b)
{
    if ( *((int*)a) < *((int*)b) )
        return -1;
    else if ( *((int*)a) > *((int*)b) )
        return 1;
    else
        return 0;
}

void printer(const void *item)
{
    printf("%d ", *((int*)item));
}

void print_arr(void *arr, int l, size_t sz, void (*printer)(const void *))
{
    char * a = (char*)arr;
    int index;
    for ( index = 0; index < l*sz; index += sz)
        printer(a+index);
    printf("\n");
}

int main(int argc, char *argv[])
{
    int a[10] = {9,6,2,3,7,4,1,0,8,5};

    print_arr(a, 10, sizeof(int), printer);

    int left, right;
    left = 4;
    right = 4;

    qsort(a+left, right-left, sizeof(int), int_compare);

    printf("%d\n", *a);
    printf("%d\n", *(a+1));

    print_arr(a, 10, sizeof(int), printer);

    char str[32];
    str[0] = '\n';
    str[1] = '\0';

    char la;
    int s, t;
    float w;
    int ret = sscanf(str, "%c %u %u %f\n", &la, &s, &t, &w);

    printf("ret=%d\n", ret);
    printf("%c %u %u %f\n", la, s, t, w);

    char line[] = "p sp 5143 12345";
    printf("%s\n",line);
    printf("strncmp=%d\n", strncmp(strtok(line," "),"sp",2));
    printf("%s\n",line);

    return 0;
}
