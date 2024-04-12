#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sched.h>
#include <pthread.h>
#include <omp.h>

#include "mt_scheme.h"
#include "graph/edgelist.h"
#include "graph/graph.h"
#include "disjoint_sets/union_find_array.h"
#include "filter-kruskal/filter_kruskal.h"
#include "util/util.h"

#ifdef PROFILE
#include "machine/tsc_x86_64.h"
tsctimer_t tim;
#endif

//int nthr; //<- npthr might be more useful
int n_main_thr;
int n_helper_thr;
pthread_barrier_t bar;

edgelist_t *el;
union_find_node_t *array;
edge_t **msf;
unsigned int msf_edges;

void *loop(void *args)
{
    while(1) {
        //hope this doesn't get optimized-out :/
    }
}

void *print(void *args)
{
    targs_t *targs = (targs_t*)args;
    int id = targs->id;
    long int from = targs->from[id];
    long int to = targs->to[id];
    int type = targs->type;

    printf("tid:%d type:%s [%ld,%ld)\n",id,(type==MAIN_THR)?"MAIN":"HELPER",from,to);

    pthread_exit(NULL);
}

// needed for quicksort.o
inline
int quicksort_cmp(_TYPE_V a, _TYPE_V b, _TYPE_AD * aux_data)
{
    return edge_compare(&a, &b);
}

__attribute__((always_inline))
static inline int threshold(int m)
{
    if ( m <= THRESHOLD )
        return 1;

    return 0;
}

__attribute__((always_inline))
static inline void swap_edge(edge_t *e1, edge_t *e2)
{
    edge_t tmp;

    tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
}

static inline int partition(edge_t *edge_array,
                            int left,
                            int right)
{
    // pick the element in the first position as the pivot :)
    edge_t pivot;
    //pivot.vertex1 = edge_array[left].vertex1;
    //pivot.vertex2 = edge_array[left].vertex2;
    //pivot.weight = edge_array[left].weight;
    pivot = edge_array[left];

    //edge_print(&pivot);
   
    // a bit ugly :( since it's an unsigned int we are possibly reducing
    // below zero (underflow), but at least it's incremented right up to
    // zero before being used as an index :)
    /*unsigned*/ int i = left-1;
    /*unsigned*/ int j = right+1;

    while (1) {
        while ( /*edge_compare(&*/edge_array[++i].weight < pivot.weight/*,&pivot) < 0*/ );
        while ( /*edge_compare(&*/edge_array[--j].weight > pivot.weight/*,&pivot) > 0*/ );
        if ( i < j ) {
            swap_edge(&edge_array[i],&edge_array[j]/*,sizeof(edge_t)*/);
            // as always, also swap corresponding edge colors :)
            //int temp = edge_color[i];
            //edge_color[i] = edge_color[j];
            //edge_color[j] = temp;
        }
        else return j;
    }
}

static
void randomized_partition_around_k(edge_t *edge_array,
                                   int left,
                                   int right,
                                   int k)
{
        
    if ( left == right )
        return;
        //return left;

    // don't foget to add the offset (+left)!
    int pivot = ((/*unsigned*/ int)(rand()) % (right-left+1)) + left;

    swap_edge(&edge_array[left],&edge_array[pivot]/*,sizeof(edge_t)*/);
    // also swap corresponding color :)
    //int temp = edge_color[left];
    //edge_color[left] = edge_color[pivot];
    //edge_color[pivot] = temp;

    // randomly partition the array around element @ pivot (which is now
    // at the left-most position due to the swap above)
#ifndef CONCURRENT_PARTITION
    int q = partition(edge_array/*,edge_color*/,left,right);
#else
    int q;
    if ( threshold(right-left+1) ) {
        q = partition(edge_array/*,edge_color*/,left,right);
    }
    else {
        #pragma omp parallel
        {
            q = quicksort_partition_concurrent_inplace(edge_array[left], edge_array, left, right, NULL);
            //q = quicksort_partition_concurrent(edge_array, buf, left, right, NULL);
        }

        if ( q > left )
            q -= 1;
    }
#endif

    int nel = q - left + 1;

    //printf("%s [%d,%d] q=%d nel=%d \n", __FUNCTION__, left, right, q, nel);

    if ( k <= nel ) return randomized_partition_around_k(edge_array/*,edge_color*/,left,q,k);
    else return randomized_partition_around_k(edge_array/*,edge_color*/,q+1,right,k-nel);
}

int main(int argc, char *argv[])
{
    if ( argc < 3 ) {
        printf("Usage: %s <graphfile> <k:nthr(mt:ht)>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

#ifdef PROFILE
    double hz;
#endif

    int nthr;
    // parse argv[2] :)
    float k = atof(strtok(argv[2],":"));
    if ( !(k > 0.0 && k < 1.0) ) {
        printf("argument 2 (k) should be in (0.0,1.0)!\n");
        exit(EXIT_FAILURE);
    }
    printf("       k=%lf\n", k);
    char *nthr_str = strtok(NULL, ":");
    if ( nthr_str ) {
        nthr = atoi(nthr_str);
        n_main_thr = 1;
        n_helper_thr = nthr-1;
        //printf("nthreads=%d\n", nthreads);
    } else {
        printf("argument 2: expected at least two (2) numbers\n"
               "            delimited by ':', <k:nthreads>;\n"
               "            or  three  (3),   <k:main:helper>\n");
        exit(EXIT_FAILURE);
    }
    nthr_str = strtok(NULL, ":");
    if ( nthr_str ) {
        n_main_thr = nthr;
        n_helper_thr = atoi(nthr_str);
        if ( n_helper_thr <= 0 ) {
            printf("argument 2: number of helper threads must be >0\n");
            exit(EXIT_FAILURE);
        }
        nthr += n_helper_thr;
        if ( nthr < 2 ) {
            printf("argument 2: total number of threads must be >=2\n");
            exit(EXIT_FAILURE);
        }
    } else if ( nthr < 2 ) {
        printf("argument 2: total number of threads must be >=2\n");
        exit(EXIT_FAILURE);
    }
    printf("threads=(%d,%d:%d)\n",nthr,n_main_thr,n_helper_thr);

    // for convenience; that's the number of pthreads we actually create :)
    int pthr_num = 1+n_helper_thr;
    
    // 1 cpuset for each helper thr (single cpu) + 1 cpuset for the main
    // thread (with n_main_thr cpus)... see below ;)
    cpu_set_t cpusets[pthr_num];

    CPU_ZERO(&cpusets[0]);
    // for now, pin 1st main thr to processor 0; we'll add cpus
    // up to n_main_thr later on :)
    int cpu_id = 0;
    CPU_SET(cpu_id,&cpusets[0]);
    // Note: on NUMA machines, all malloc calls that follow
    // a _setaffinity system call reserve pages on the memory node 
    // which is local to the affine processor. For this reason, we 
    // should bind the main context (i.e. the context where all 
    // allocations occur) to the same processor as the main thread 
    // will run on later, before any allocations.
    sched_setaffinity(getpid(), sizeof(cpusets[0]), &cpusets[0]);

    targs_t *targs;
    pthread_t *tids;
    pthread_attr_t *attr;

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    // Allocate thread structures
    tids = (pthread_t*)malloc_safe(pthr_num*sizeof(pthread_t));
    targs = (targs_t*)malloc_safe(pthr_num*sizeof(targs_t));
    long int *from = (long int*)malloc_safe(pthr_num*sizeof(long int));
    long int *to = (long int*)malloc_safe(pthr_num*sizeof(long int));
    long int *e = (long int*)malloc_safe(pthr_num*sizeof(long int));
    attr = (pthread_attr_t*)malloc_safe(pthr_num*sizeof(pthread_attr_t)); 
    pthread_barrier_init(&bar, NULL, pthr_num);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "thr struct allocate    cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    int is_undirected = 0;
    char *inp_method;
#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    //al = adjlist_read(graphfile, &stats, is_undirected);
    el = edgelist_choose_input_method(argv[1],is_undirected,0,&inp_method);
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "edgelist_%s          cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    inp_method,
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif
    free(inp_method);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
    //TODO: this could be done by the MAIN_THR, while the HELPER_THRs 
    // compute their bounds ;)
    kruskal_ht_scheme_init();
    
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "kruskal init           cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    long int main_region = /*lround(*/el->nedges * k/*) / 100*/;
    long int ht_region = el->nedges-main_region;
    printf("main_region = [0,%ld)\n", main_region);
    printf("ht_region = [%ld,%ld)\n", main_region, main_region+ht_region);

    assert( main_region+ht_region == el->nedges );

    // temprarily give all available cpus to main thr 
    for ( cpu_id = 1; cpu_id < nthr; cpu_id++ )
        CPU_SET(cpu_id,&cpusets[0]);
    sched_setaffinity(getpid(), sizeof(cpusets[0]), &cpusets[0]);

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif
#ifdef CONCURRENT_PARTITION
    //printf("setting omp_num_threads to %d\n",nthr);
    omp_set_num_threads(nthr);
#endif
    randomized_partition_around_k(el->edge_array,0,el->nedges-1,main_region);
    
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "quickselect            cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

#ifdef PROFILE
    timer_clear(&tim);
    timer_start(&tim);
#endif

    int i;

    // Compute thread bounds :)
    long int chunk_size = ht_region / n_helper_thr ;
    int rem = ht_region % n_helper_thr ;

    for ( i = 0; i < pthr_num; i++ ) {
        if ( i < 1 ) {
            targs[i].type = MAIN_THR;
            from[i] = 0;
            to[i] = main_region;
            CPU_ZERO(&cpusets[i]);
            for ( cpu_id = 0; cpu_id < n_main_thr; cpu_id++ )
                CPU_SET(cpu_id,&cpusets[0]);
        } else {
            targs[i].type = HELPER_THR;
            from[i] = main_region + ( (i-1<=rem) ? (i-1)*(chunk_size+1) : rem*(chunk_size+1)+(i-1-rem)*chunk_size );
            to[i] = from[i] + chunk_size + ( i-1 <= rem-1 ); 
            CPU_ZERO(&cpusets[i]);
            CPU_SET(n_main_thr+i-1,&cpusets[i]);
        }
        targs[i].id = i;
        // every thread gets to know the bounds of every other :)
        // (from and to arrays are populated progressively)
        targs[i].from = from;
        targs[i].to = to;
        //printf("tid#%d [%ld,%ld)\n", targs[i].id, targs[i].from[i], targs[i].to[i]);
        targs[i].e = e;
        targs[i].cycles_found = 0;

        pthread_attr_init(&attr[i]);
        pthread_attr_setaffinity_np(&attr[i],
                                    sizeof(cpusets[i]),
                                    &cpusets[i]);
        pthread_create(&tids[i],&attr[i],kruskal_ht_scheme/*print*/,(void*)&targs[i]);
    }

    for ( i = 0; i < pthr_num; i++ ) {
        pthread_join(tids[i],NULL);
        //printf("tid#%d cycles_found:%ld\n",targs[i].id,targs[i].cycles_found);
        pthread_attr_destroy(&attr[i]);
    }
#ifdef PROFILE
    timer_stop(&tim);
    hz = timer_read_hz();
    fprintf(stdout, "pthreads fork/join     cycles:%18.2lf seconds:%10lf freq:%lf\n", 
                    timer_total(&tim),
                    timer_total(&tim) / hz,
                    hz );
#endif

    weight_t msf_weight = 0.0;
    for ( i = 0; i < msf_edges; i++ )
        msf_weight += msf[i]->weight;
    
    printf("msf_edges=%u\n", msf_edges);
    printf("msf_weight=%f\n", msf_weight);

    // clean-up things
    pthread_barrier_destroy(&bar);
    free(tids);
    free(targs);
    free(from);
    free(to);
    free(attr);

    kruskal_ht_scheme_destroy();
    edgelist_destroy(el);

    return 0;
}
