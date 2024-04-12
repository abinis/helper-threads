#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edgelist.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
#endif

int main(int argc, char *argv[])
{
    if ( argc < 2 ) {
        printf("usage: %s <graphfile>\n", argv[0]);
        exit(0);
    }

#ifdef PROFILE
    tsctimer_t tim;
    double hz;
#endif

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    edgelist_t *el = edgelist_read(argv[1],0,0);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_read          cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    printf("sizeof edge_t* = %lu\n", sizeof(edge_t*));
    printf("sizeof weight_t = %lu\n", sizeof(weight_t));
    printf("sizeof edgelist_t = %lu\n", sizeof(edgelist_t));

    char *tok = strtok(argv[1],"/");
    char *next_tok;
    while ( (next_tok = strtok(NULL,"/")) != NULL )
        tok = next_tok;
    char dumpfile[256];
    strncpy(dumpfile,tok,250);
    strcat(dumpfile,".dump");
    printf("tok=%s, dumpfile=%s\n", tok, dumpfile);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    int ok = edgelist_dump(el, dumpfile);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_dump          cycles:%lf seconds:%lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    if ( !ok )
        perror("edgelist_dump failed!");

    edgelist_destroy(el);

//#ifdef PROFILE
//    timer_clear(&tim);
//    timer_start(&tim);
//#endif
//    edgelist_t *loaded = edgelist_load(dumpfile);
//    //edgelist_t *loaded = edgelist_load(argv[1]);
//#ifdef PROFILE
//    timer_stop(&tim);
//    hz = timer_read_hz();
//    fprintf(stdout, "edgelist_load          cycles:%lf seconds:%lf freq:%lf\n", 
//                    timer_total(&tim),
//                    timer_total(&tim) / hz,
//                    hz );
//#endif
//
//    //edgelist_print(loaded);
//
//    edgelist_destroy(loaded);

    return 0;
}
