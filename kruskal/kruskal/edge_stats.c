#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "edge_stats.h"

void * calloc_safe(size_t size)
{
	void *p;

	if ( !(p = malloc(size)) ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
		exit(EXIT_FAILURE);
	}

        memset(p, 0, size);

	return p;
}

struct gap_list_node {
    unsigned int gap_length;
    unsigned int gap_count;
    struct gap_list_node * next;
};

struct gap_list_node * gap_list_create_empty()
{
    struct gap_list_node * glp;

    glp = calloc_safe(sizeof(struct gap_list_node));

    glp->gap_length = 0;
    glp->gap_count = 0;
    glp->next = NULL;
    
   // glp = NULL;

    return glp;
}

struct gap_list_node * gap_list_new_node(unsigned int l)
{
    struct gap_list_node * glp;

    glp = calloc_safe(sizeof(struct gap_list_node));

    glp->gap_length = l;
    glp->gap_count = 1;
    glp->next = NULL;

    return glp;
}

unsigned int find_nextpow2(unsigned int v)
{
    unsigned int ret;

    // Count leading zeroes
    int lz = __builtin_clz(v);
    // v is a power of 2 (and non-zero)
    if ( v && !(v & (v-1)) )
        ret = v;
    else
        ret = 1U << (32-lz);

    return ret;
}

void gap_list_update_pow2(struct gap_list_node *glp, unsigned int gaplength)
{
    assert(glp);

    struct gap_list_node *p, *prev;

    p = glp;
    prev = p;

    // Gap lengths are grouped in buckets according to the nearest power of 2
    unsigned int nextpow2 = find_nextpow2(gaplength);

    // List is "empty": update 0 to nextpow2, count to 1, done
    if ( p->gap_length == 0 ) {
        p->gap_length = nextpow2;
        p->gap_count = 1;
        //printf("first node! (%u, %u)\n", p->gap_length, p->gap_count);
        return;
    }

    // Otherwise, locate which node needs updating
    while ( p != NULL ) {
        if ( p->gap_length == nextpow2 ) {
            (p->gap_count)++;
            //printf("node updated! (%u, %u)\n", p->gap_length, p->gap_count);
            return;
        } 
        if ( p->gap_length > nextpow2 ) {
            break;
        }
        prev = p;
        p = p->next;
    }

    // We need to create a new node :)
    struct gap_list_node * new = gap_list_new_node(nextpow2);
    // case 1: new node goes exactly before p
    if ( p != NULL ) {
        new->next = p;
        if ( glp == p ) // case 1-i: at the start :)
            glp = new;
        else // case 1-ii: somewhere inbetween :P
            prev->next = new;
    } else { // case 2: new node goes at the end
        prev->next = new;
    }
    //printf("node created! (%u, %u)\n", new->gap_length, new->gap_count);
}

void gap_list_update(struct gap_list_node *glp, unsigned int gaplength)
{
    assert(glp);

    struct gap_list_node *p, *prev;

    p = glp;
    prev = p;

    // List is "empty": update 0 to gaplength, count to 1, done
    if ( p->gap_length == 0 ) {
        p->gap_length = gaplength;
        p->gap_count = 1;
        //printf("first node! (%u, %u)\n", p->gap_length, p->gap_count);
        return;
    }

    // Otherwise, locate which node needs updating
    while ( p != NULL ) {
        if ( p->gap_length == gaplength ) {
            (p->gap_count)++;
            //printf("node updated! (%u, %u)\n", p->gap_length, p->gap_count);
            return;
        } 
        if ( p->gap_length > gaplength ) {
            break;
        }
        prev = p;
        p = p->next;
    }

    // We need to create a new node :)
    struct gap_list_node * new = gap_list_new_node(gaplength);
    // case 1: new node goes exactly before p
    if ( p != NULL ) {
        new->next = p;
        if ( glp == p ) // case 1-i: at the start :)
            glp = new;
        else // case 1-ii: somewhere inbetween :P
            prev->next = new;
    } else { // case 2: new node goes at the end
        prev->next = new;
    }
    //printf("node created! (%u, %u)\n", new->gap_length, new->gap_count);
}

void gap_list_print(struct gap_list_node *glp)
{
    assert(glp);

    struct gap_list_node * p;
    unsigned int max = glp->gap_count;
    unsigned int scale = 60;
    unsigned int total_gaps = 0;
    unsigned int sum_gaps = 0;

    p = glp;
    while ( p != NULL ) {
        if ( p->gap_count > max )
            max = p->gap_count;
        total_gaps += p->gap_count;
        p = p->next;
    }
    
    p = glp;    
    while ( p != NULL ) {
        //printf("(%u,%u)\n", p->gap_length, p->gap_count);
        sum_gaps += p->gap_count;
        printf("%10u %8.4lf%%  ", p->gap_length, ((double)sum_gaps / (double)total_gaps)*100);
        int i;
        for ( i = 0; i < (p->gap_count)*scale / max; i++ )
            printf("#");
        if ( (p->gap_count)*scale / max == 0 )
            printf(".");
        printf(" %u(%.4lf%%)\n", p->gap_count, ((double)(p->gap_count) / (double)total_gaps)*100);
        p = p->next;
    }
}

void gap_list_destroy(struct gap_list_node *glp)
{
    assert(glp);

    struct gap_list_node *p, *np;

    p = glp;
    while ( p != NULL ) {
        np = p->next;
        free(p);
        p = np;
    }
}

void print_edge_stats(unsigned int len, int *edge_membership,
                      int total_msf_edges, int total_cycle_edges)
{
    unsigned int i;
    unsigned int *datapoints;
    double *msf_edge_percentage;
    double *cycle_edge_percentage;
    double *msf_edge_percentage_t;
    double *cycle_edge_percentage_t;

    datapoints = calloc_safe( NUM_DATAPOINTS * sizeof(unsigned int) );
    msf_edge_percentage = calloc_safe( NUM_DATAPOINTS * sizeof(double) );
    cycle_edge_percentage = calloc_safe( NUM_DATAPOINTS * sizeof(double) );
    msf_edge_percentage_t = calloc_safe( NUM_DATAPOINTS * sizeof(double) );
    cycle_edge_percentage_t = calloc_safe( NUM_DATAPOINTS * sizeof(double) );
/*
    datapoints = calloc(NUM_DATAPOINTS, sizeof(unsigned int));
    if (!datapoints) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    msf_edge_percentage = calloc(NUM_DATAPOINTS, sizeof(double));
    if (!msf_edge_percentage) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    cycle_edge_percentage = calloc(NUM_DATAPOINTS, sizeof(double));
    if (!cycle_edge_percentage) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
*/
    unsigned int chunk = len / NUM_DATAPOINTS;
    int rem = len % NUM_DATAPOINTS;
    for ( i = 1; i <= NUM_DATAPOINTS; i++ ) {
        datapoints[i-1] = ( i <= rem ) ? i*(chunk+1) : rem*(chunk+1)+(i-rem)*chunk;
    }

    unsigned int msf_edge_count = 0;
    unsigned int msf_edge_count_p = 0;
    unsigned int cycle_edge_count = 0;
    unsigned int cycle_edge_count_p = 0;
    int point = 0;

    // compute the number of digits of 'len',
    // we need this for the field width of the unsigned ints below :)
    unsigned int width = 1;
    unsigned int number = len;
    while ( number / 10 > 0 ) {
        width++;
        number /= 10;
    }
    printf("width=%u\n", width);
    if ( width < 8 ) width = 8;

    printf("%3s %*s %*s %*s %8s %8s %*s %8s %*s %8s\n", "%", width, "edge#", width, "msf#p", width, "cycle#p", "msf%", "cycle%", width, "msf#", "msftot%", width, "cyc#", "cyctot%");
    for ( i = 0; i < len; i++ ) {

        if ( edge_membership[i] == 1 ) {
            msf_edge_count++;
            // counts the msf edges *within* current percentile :)
            msf_edge_count_p++;
        }

        if ( edge_membership[i] == 2 ) {
            cycle_edge_count++;
            // similarly, counts the cycle edges within current percentile ;)
            cycle_edge_count_p++;
        }

        if ( i == datapoints[point]-1 ) {
            assert( (msf_edge_count+cycle_edge_count) == i+1 );
            assert( (msf_edge_count_p+cycle_edge_count_p) == datapoints[point] - datapoints[point-1] );
            msf_edge_percentage[point] = (double)msf_edge_count / (double)(i+1);
            cycle_edge_percentage[point] = (double)cycle_edge_count / (double)(i+1);
            msf_edge_percentage_t[point] = (double)msf_edge_count / (double)total_msf_edges;
            cycle_edge_percentage_t[point] = (double)cycle_edge_count / (double)total_cycle_edges;
            printf("%3u %*u %*u %*u %lf %lf %*u %lf %*u %lf\n",
                   point+1, width, datapoints[point], width, msf_edge_count_p, width, cycle_edge_count_p,
                                       msf_edge_percentage[point], cycle_edge_percentage[point],
                                       width, msf_edge_count, msf_edge_percentage_t[point],
                                       width, cycle_edge_count, cycle_edge_percentage_t[point]); 

            // we advance the point, and reset the within-percentile counters :)
            point++;
            msf_edge_count_p = 0;
            cycle_edge_count_p = 0;
        }

    }

    // moved up, inside loop above!
    //printf("%3s %8s %8s %8s %8s %8s %8s %8s\n", "%", "upToEdge", "msf%", "cycle%", "msf#", "msftot%", "cyc#", "cyctot%");
    //for ( i = 0; i < NUM_DATAPOINTS; i++ )
    //    printf("%3u %8u %lf %lf %lf %lf\n",
    //           i+1, datapoints[i], msf_edge_percentage[i], cycle_edge_percentage[i],
    //                               
    //                               msf_edge_percentage_t[i], cycle_edge_percentage_t[i]); 

    free(datapoints);
    free(msf_edge_percentage);
    free(cycle_edge_percentage);
    free(msf_edge_percentage_t);
    free(cycle_edge_percentage_t);
    
}

void print_gap_distribution(unsigned int len, int *edge_membership)
{
    unsigned int i;
    struct gap_list_node * gap_list;

    gap_list = gap_list_create_empty();

    unsigned int gap_length = 0;
    for ( i = 0; i < len; i++ ) {

        if ( edge_membership[i] != 2 ) {
            if ( gap_length > 0 ) {
                //gap_list_update(gap_list, gap_length);
                gap_list_update_pow2(gap_list, gap_length);
                gap_length = 0;
            }
            continue;
        }

        gap_length++;

    }
    // "flush" last measurement :)
    if ( gap_length > 0 ) 
        //gap_list_update(gap_list, gap_length);
        gap_list_update_pow2(gap_list, gap_length);

    gap_list_print(gap_list);
    gap_list_destroy(gap_list);
}
