#ifndef MT_SCHEME_H_
#define MT_SCHEME_H_

#include "dgal-quicksort/quicksort.h"

enum { MAIN_THR = 1, HELPER_THR };

typedef struct {
    int id;
    int type;
    long int *from;
    long int *to;
    long int *e;
    long int cycles_found;
} targs_t;

void kruskal_ht_scheme_init(void);
void * kruskal_ht_scheme(void *args);
void kruskal_ht_scheme_destroy(void);

#endif
