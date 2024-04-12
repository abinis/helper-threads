#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

int main (int argc, char *argv[])
{
    int next_option, print_flag, stats_flag, oracle_flag, sim_flag;
    char graphfile[256];
    // used with --simulate (shorthand 's') option
    //char kstring[32];
    int k;
    int nthreads;
    
    if ( argc == 1 ) {
        printf("Usage: %s --graph <graphfile>\n"
                "   [--print]\n"
                "   [--stats]\n"
                "   [--oracle]\n"
                "   [--simulate]\n"
                  , argv[0]);
        exit(EXIT_FAILURE);
    }

    print_flag = 0;
    stats_flag = 0;
    oracle_flag = 0;
    sim_flag = 0;

    /* getopt stuff */
    const char* short_options = "g:psom:";
    const struct option long_options[]={
        {"simulate", 1, NULL, 'm'},
        {"oracle", 2, NULL, 'o'},
        {"stats", 2, NULL, 's'},
        {"graph", 1, NULL, 'g'},
        {"print", 0, NULL, 'p'},
        {NULL, 0, NULL, 0}
    };

    do {
        next_option = getopt_long(argc, argv, short_options, long_options, 
                                  NULL);
        switch(next_option) {
            case 'p':
                print_flag = 1;
                break;

            case 'g':
                sprintf(graphfile, "%s", optarg);
                break;
            
            case 's':
                stats_flag = 1;
                break;
            
            case 'o':
                oracle_flag = 1;
                break;

            case 'm':
                sim_flag = 1;
                //sprintf(kstring, "%s", optarg);
                printf("  optarg=%s\n", optarg);
                k = atoi(strtok(optarg, ":"));
                printf("       k=%d\n", k);
                unsigned int nedges = 11820688;
                printf("    upto=%u\n", nedges*k/100 );
                char *nthr_str = strtok(NULL, ":");
                if ( nthr_str ) {
                    nthreads = atoi(nthr_str);
                    printf("nthreads=%d\n", nthreads);
                } else {
                    printf("--simulate: expected two (2) arguments separated\n"
                           "            by a colon ':', <k:nthreads>\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case '?':
                fprintf(stderr, "Unknown option!\n");
                exit(EXIT_FAILURE);

            case -1:    // Done with options
                break;  

            default:    // Unexpected error
                exit(EXIT_FAILURE);
        }

    } while ( next_option != -1 );

    return 0;
}
