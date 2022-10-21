/**
 * @file
 * Test processor_map.c
 */

#include <assert.h>
#include <stdio.h>

#include "processor_map/processor_map.h"

int main (int argc, char *argv[])
{
    procmap_t *pi;
   
    pi = procmap_init();
    assert(pi);

    procmap_report(pi);

    procmap_destroy(pi);

    return 0;
}
