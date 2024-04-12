#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <pthread.h>

//struct args_st {
//    void *ptr;
//};

void *loop_infinitely(void *args)
{
    while(1) {
        //hope this doesn't get optimized-out :/
    }
}

int main (int argc, char *argv[])
{
    if ( argc < 2 ) {
        printf("usage: %s <num_cpus>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    pthread_t tid;
    pthread_attr_t attr;
    //struct args_st args;

    cpu_set_t cpusets[10];

    printf("sizeof(cpu_set_t)=%lu\n", sizeof(cpusets[0]));

    CPU_ZERO(&cpusets[0]);
    //CPU_SET(1, &cpusets[0]);
    CPU_SET(22, &cpusets[0]);

    pthread_attr_init(&attr);
    pthread_attr_setaffinity_np(&attr,sizeof(cpusets[0]),&cpusets[0]);

    pthread_create(&tid,&attr,loop_infinitely,NULL);

    pthread_join(tid,NULL);

    return 0;
}
