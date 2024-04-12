/**
 * @file
 * Definitions for functions related to processor hierarchy
 * exploration.
 */ 

#include "processor_map.h"

#include <assert.h>
#include <error.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wait.h>

#include "util/util.h"

/**
 * @return Total number of cpus (hw threads) of the system, 
 *         0 if no info found
 */ 
static inline int _get_num_cpus() 
{
    if ( open("/sys/devices/system/cpu/", O_RDONLY ) < 0 )
        return 0;
    else
        return WEXITSTATUS(
            system("exit $(ls -l /sys/devices/system/cpu/"
                   " | grep 'cpu[0-9]*[0-9]' | wc -l)")
            );
}

/**
 * @return Total number of cache levels that cpu0 (and every
 *         other cpu) sees, 0 if no info found
 */         
static inline int _get_num_caches() 
{
    if ( open("/sys/devices/system/cpu/cpu0/cache/", O_RDONLY ) < 0 )
        return 0;
    else
        return WEXITSTATUS(
            system("exit $(ls -l /sys/devices/system/cpu/cpu0/cache/"
                   " | grep index | wc -l)")
            );
}

/**
 * @return Total number of memory nodes of the system, 
 *         0 if no info found.
 */ 
static inline int _get_num_memnodes()
{
    if ( open("/sys/devices/system/node/", O_RDONLY ) < 0 )
        return 0;
    else
        return WEXITSTATUS(
            system("exit $(ls -l /sys/devices/system/node/"
                   " | grep node | wc -l)")
            );
}

/**
 * Initialises the contents of an array of unsigned integers (used
 * for storing a bitmask) to zero
 * @return 1 for success, 0 for failure (NULL pointer basically)
 */
int bitmask_arr_clear (unsigned int *arr)
{
    if (arr == NULL)
        return 0;

    memset(arr, 0, BITMASK_ARR_LEN*sizeof(unsigned int));

    return 1;
}

/**
 * Parses the string of hex digits pointed to by hexbuf
 * into an array of unsigned ints. Every 8 characters (bytes)
 * of the hex string translate to a 4-byte unsigned int.
 */
void bitmask_arr_fill_from_hexbuf (unsigned int *arr, char *hexbuf)
{
    char buf[16];
    memset(buf, 0, 16);

    /**
     * In case the length of the hex string isn't a multiple
     * of eight, pad the front with zeroes (that's actual zero
     * characters :P) until it is :)
     */
    char padded_hexbuf[64];
    memset(padded_hexbuf, 0, 64);
    int offset = 8 - strlen(hexbuf)%8;
    strncpy(padded_hexbuf+offset, hexbuf, strlen(hexbuf));
    int i = 0;
    while ( i < offset )
        padded_hexbuf[i++] = '0';

    // Find where inside the padded buffer the hex string ends
    int end = 0;
    while (padded_hexbuf[end] != '\0')
        end++;

    // Parse the hex string, from end to start, 8 hex digits
    // at a time, converting them to ints, storing them into arr
    int j = 0;
    for (i = end-8; i >= 0; i-=8) {
        strncpy(buf, padded_hexbuf+i, 8);
        arr[j] = strtol(buf, NULL, 16);
        j++;
        if ( j >= BITMASK_ARR_LEN ) {
            fprintf(stderr, "Bitmask array out of space!\n");
            break;
        }
        memset(buf, 0, 16);
    }
}

void bitmask_arr_show_contents (unsigned int *arr)
{
    int i;
    const unsigned int msb_set = 0x80000000;
    for (i = 0; i < BITMASK_ARR_LEN; i++) {
       unsigned int num = arr[i];
           printf("%10u:", num);
       int j = 0;
       //Each (unsigned) int is 32 bits long
       while ( j++ < 32 ) {
          if ( (j-1) % 4 == 0 )
             printf(" ");
          printf("%c", ( num & msb_set ) ? '1' : '0');
          num <<= 1;
       }
       printf("\n");
    }
}

void bitmask_show (unsigned int bitmask)
{
    int i = 0;
    unsigned int num = bitmask;
    const unsigned int msb_set = 0x80000000;
    while ( i++ < 32 ) {
       if ( (i-1) % 4 == 0 )
          printf(" ");
       printf("%c", ( num & msb_set ) ? '1' : '0');
       num <<= 1;
    }
    printf("\n");
}

/**
 * Allocates memory for procmap structures and populates
 * them.
 * @return handle to the procmap basic structure
 */  
procmap_t* procmap_init() 
{
    procmap_t *pi;

    char base_path[] = "/sys/devices/system/";
    char path[128], 
         curr_cpu[8],
         curr_index[12],
         curr_node[8],
         cpu_path[128],
         buf[1024];
    int fd, i, j, k, l, br, 
        found_so_far, 
        curr_id,
        num_cpus,
        num_memnodes,
        num_caches, 
        num_packages, 
        num_cores_per_package,
        num_threads_per_core;
    int *sym_pack_ids,
        *sym_core_ids;
    cacheinfo_t *cache;
    coreinfo_t *core;

    path[0] = '\0';
    strcat(path, base_path);
    strcat(path, "/cpu/");
    if ( open(path, O_RDONLY) < 0 ) {
        perror(base_path);
        fprintf(stderr, "Required information was not found. Exiting\n");
        exit(EXIT_FAILURE);
    }

    path[0] = '\0';
    strcat(path, base_path);
    strcat(path, "/cpu/cpu0/topology/");
    if ( open(path, O_RDONLY) < 0 ) {
        fprintf(stderr, "System does not provide processor topology info\n");
    } 

    path[0] = '\0';
    strcat(path, base_path);
    strcat(path, "/cpu/cpu0/cache/");
    if ( open(path, O_RDONLY) < 0 ) {
        fprintf(stderr, "System does not provide processor cache info\n");
    } 

    path[0] = '\0';
    strcat(path, base_path);
    strcat(path, "/node/node0/");
    if ( open(path, O_RDONLY) < 0 ) {
        fprintf(stderr, "System does not provide memory node info\n");
    } 

    // Allocate handle
    //printf("SIZE OF procmap_t = %lu\n", sizeof(procmap_t));
    pi = (procmap_t*)malloc(sizeof(procmap_t));
    if ( !pi ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    num_cpus = _get_num_cpus();
    pi->num_cpus = num_cpus;
   
    num_memnodes = _get_num_memnodes();
    pi->num_memnodes = num_memnodes;
    
    num_caches = _get_num_caches();
    pi->num_caches_per_thread = num_caches;

    // Array of handles for all hw threads of the system
    threadinfo_t *flat_threads = (threadinfo_t*)malloc(num_cpus * 
                                                       sizeof(threadinfo_t));
    if ( !flat_threads ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    pi->flat_threads = num_cpus > 0 ? flat_threads : NULL;

    // populate array
    for ( i = 0; i < num_cpus; i++ ) {
        flat_threads[i].cpu_id = i;
        
        sprintf(curr_cpu, "cpu%d", i);
        cpu_path[0] = '\0';
        strcat(cpu_path, base_path);
        strcat(cpu_path, "/cpu/");
        strcat(cpu_path, curr_cpu);
       
        // Get core id
        path[0] = '\0';
        strcat(path, cpu_path);
        strcat(path, "/topology/core_id"); 
        if ( (fd = open(path, O_RDONLY)) < 0 ) {
            fprintf(stderr, "Could not open %s\n", path);
            flat_threads[i].sym_core_id = -1;
        } else {
            br = read(fd, buf, 1024);
            buf[br] = 0;
            trim(buf, '\n');
            flat_threads[i].sym_core_id = strtol(buf, NULL, 10);
            close(fd);
        }

        // Get package id
        path[0] = '\0';
        strcat(path, cpu_path);
        strcat(path, "/topology/physical_package_id"); 
        if ( (fd = open(path, O_RDONLY)) < 0 ) {
            fprintf(stderr, "Could not open %s\n", path);
            flat_threads[i].sym_pack_id = -1;
        } else {
            br = read(fd, buf, 1024);
            buf[br] = 0;
            trim(buf, '\n');
            flat_threads[i].sym_pack_id = strtol(buf, NULL, 10);
            close(fd);
        }

        // Get core siblings map
        path[0] = '\0';
        strcat(path, cpu_path);
        strcat(path, "/topology/core_siblings"); 
        //init core_siblings[16];
	//printf("cpuNo #%d\n", i);
        bitmask_arr_clear(flat_threads[i].core_siblings);
        //bitmask_arr_show_contents(flat_threads[i].core_siblings);
        //memset(flat_threads[i].core_siblings, 0, 16*sizeof(unsigned int));
        if ( (fd = open(path, O_RDONLY)) < 0 ) {
            fprintf(stderr, "Could not open %s\n", path);
            //Swapping int type with int array, a few changes are needed :)
            //This shouldn't work, core_siblings is of type
            //unsigned int for the whole bitmask thing to work
            //flat_threads[i].core_siblings[0] = -1;
            //snprintf(flat_threads[i].core_siblings, 32, "%d", -1);
        } else {
            br = read(fd, buf, 1024);
            buf[br] = 0;
            //printf("Siblings buffer read for cpu%d\n--before trim\n", i);
            //puts(buf);
            trim(buf, ',');
            trim(buf, '\n');
            //printf("--after trim\n");
            //puts(buf);
            bitmask_arr_fill_from_hexbuf(flat_threads[i].core_siblings, buf);
            //bitmask_arr_show_contents(flat_threads[i].core_siblings);
            //flat_threads[i].core_siblings = (long long int)strtoll(buf, NULL, 16);
            //snprintf(flat_threads[i].core_siblings, 32, "%s", buf);
            //printf("core siblings for cpu%d: %s\n", i, flat_threads[i].core_siblings);
            close(fd);
        }

        // Get thread siblings map
        path[0] = '\0';
        strcat(path, cpu_path);
        strcat(path, "/topology/thread_siblings"); 
        bitmask_arr_clear(flat_threads[i].thread_siblings);
        if ( (fd = open(path, O_RDONLY)) < 0 ) {
            fprintf(stderr, "Could not open %s\n", path);
            //This shouldn't work, thread_siblings is of type
            //unsigned int for the whole bitmask thing to work
            //flat_threads[i].thread_siblings[0] = -1;
            //snprintf(flat_threads[i].thread_siblings, 32, "%d", -1);
        } else {
          br = read(fd, buf, 1024);
            buf[br] = 0;
            trim(buf, ',');
            trim(buf, '\n');
            //puts(buf);
            bitmask_arr_fill_from_hexbuf(flat_threads[i].thread_siblings, buf);
            //flat_threads[i].thread_siblings = (long long int)strtoll(buf, NULL, 16);
            //snprintf(flat_threads[i].thread_siblings, 32, "%s", buf);
            //printf("thread siblings for cpu%d: %s\n", i, flat_threads[i].thread_siblings);
            close(fd);
        }

        // Get cache info
        flat_threads[i].num_caches = num_caches;
        //printf("flat_threads[%d].num_caches = %d\n", i, flat_threads[i].num_caches);
	//printf("SIZE OF cacheinfo_t = %lu\n", sizeof(cacheinfo_t));
        cache = (cacheinfo_t*)malloc(num_caches * sizeof(cacheinfo_t));
        if ( !cache ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }

        for ( j = 0; j < num_caches; j++ ) {
            
            sprintf(curr_index, "/index%d/", j);

            path[0]='\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/coherency_line_size");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                cache[j].coherency_line_size = -1;
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                cache[j].coherency_line_size = strtol(buf, NULL, 10);
                close(fd);
            }
        
            path[0]='\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/level");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                cache[j].level = -1;
            } else {
               br = read(fd, buf, 1024);
               buf[br] = 0;
               trim(buf, '\n');
               cache[j].level = strtol(buf, NULL, 10);
               close(fd);
            }

            path[0]='\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/number_of_sets");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                cache[j].number_of_sets = -1;
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                cache[j].number_of_sets = strtol(buf, NULL, 10);
                close(fd);
            }

            path[0]='\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/physical_line_partition");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                cache[j].physical_line_partition = -1;
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                cache[j].physical_line_partition = strtol(buf, NULL, 10);
                close(fd);
            }

            path[0]='\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/shared_cpu_map");
            bitmask_arr_clear(cache[j].shared_cpu_map);
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                //cache[j].shared_cpu_map = -1;
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                trim(buf, ',');
                trim(buf, ',');
                //cache[j].shared_cpu_map = strtol(buf, NULL, 16);
                bitmask_arr_fill_from_hexbuf(cache[j].shared_cpu_map, buf);
                close(fd);
            }

            path[0] = '\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/size");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                cache[j].size = -1;
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                trim(buf, 'K');
                cache[j].size = strtol(buf, NULL, 10)*1024;
                close(fd);
            }

            path[0] = '\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/type");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                sprintf(cache[j].type, "-1");
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                strcpy(cache[j].type, buf); 
                close(fd);
            }

            path[0]='\0';
            strcat(path, cpu_path);
            strcat(path, "/cache/");
            strcat(path, curr_index);
            strcat(path, "/ways_of_associativity");
            if ( (fd = open(path, O_RDONLY)) < 0 ) {
                fprintf(stderr, "Could not open %s\n", path);
                cache[j].ways_of_associativity = -1;
            } else {
                br = read(fd, buf, 1024);
                buf[br] = 0;
                trim(buf, '\n');
                cache[j].ways_of_associativity = strtol(buf, NULL, 10);
                close(fd);
            }
       
        } // for each cache
       
        flat_threads[i].cache = num_caches > 0 ? cache : NULL;

    } // for each cpu
            
    /* sym_pack_ids: contains all different package symbolic id's 
     * Its size can be at most num_cpus (i.e. in case each cpu 
     * constitutes a different package). 
     * The list of all different package symbolic id's ends either 
     * when a -1 is found or when we have reached the max number 
     * of cpus.
     */
    sym_pack_ids = (int*)malloc(num_cpus * sizeof(int));
    if ( !sym_pack_ids ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    for ( i = 0; i < num_cpus; i++ )
        sym_pack_ids[i] = -1;

    // find num_packages and populate sym_pack_ids
    num_packages = 0;

    for ( i = 0; 
          i < num_cpus && 
              (curr_id = flat_threads[i].sym_pack_id) != -1; 
          i++ ) {

        found_so_far = 0;
        for ( j = 0; j < i; j++ ) {
            if ( flat_threads[j].sym_pack_id == curr_id ) {
                found_so_far = 1;
                break;
            }
        }
        
        // new id
        if ( !found_so_far ) 
            sym_pack_ids[num_packages++] = curr_id;
        
    }
    pi->num_packages = num_packages;

    // populate pack_ids
    for ( i = 0; i < num_cpus; i++ ) {
        flat_threads[i].pack_id = -1;

        for ( j = 0; j < num_cpus; j++ ) {
            if ( sym_pack_ids[j] == -1 )
                break;
            if ( flat_threads[i].sym_pack_id == sym_pack_ids[j] )
                flat_threads[i].pack_id = j;
        }
    }

    /* sym_core_ids: contains all different core symbolic id's
     * Its size can be at most num_cpus (i.e. in case each cpu 
     * constitutes a different core). 
     * The list of all different core symbolic id's ends either
     * when a -1 is found, or when we have reached the max number 
     * of cpus
     */
    sym_core_ids = (int*)malloc(num_cpus * sizeof(int));
    if ( !sym_core_ids ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    for ( i = 0; i < num_cpus; i++ )
        sym_core_ids[i] = -1;

    // find num_cores_per_package and populate sym_core_ids
    num_cores_per_package = 0;
    
    for ( i = 0; 
          i < num_cpus && (curr_id = flat_threads[i].sym_core_id) != -1;
          i++ ) {

        found_so_far = 0;
        for ( j = 0; j < i; j++ ) {
            if ( flat_threads[j].sym_core_id == curr_id ) {
                found_so_far = 1;
                break;
            }
        }

        // new id
        if ( !found_so_far ) 
            sym_core_ids[num_cores_per_package++] = curr_id;
    }
    pi->num_cores_per_package = num_cores_per_package;

    // populate core_ids
    for ( i = 0; i < num_cpus; i++ ) {
        flat_threads[i].core_id = -1;

        for ( j = 0; j < num_cpus; j++ ) {
            if ( sym_core_ids[j] == -1 ) 
                break;
            if ( flat_threads[i].sym_core_id == sym_core_ids[j] )
                flat_threads[i].core_id = j;
        }
    }

    // find num_threads_per_core 
    // check the first cpu for additional thread siblings
    //printf("***num_cpus = %d\n", num_cpus);
    num_threads_per_core = 0;
    for ( i = 0; i < num_cpus; i++ ) {
        int my_chunk = i / (8*sizeof(unsigned int));
        int offset = i % (8*sizeof(unsigned int));
        //printf("cpu#%d, my chunk is %d and my offset %d\n", i, my_chunk, offset);
        //printf("        ");
        //bitmask_show(flat_threads[0].thread_siblings[my_chunk]);
        if ( (1<<offset) & flat_threads[0].thread_siblings[my_chunk] )
            num_threads_per_core++;
        //for ( j = 0; j < BITMASK_ARR_LEN; j++) {
        //    if ( (1<<i) & flat_threads[0].thread_siblings[j] )
        //        num_threads_per_core++;
        //}
    }
    pi->num_threads_per_core = num_threads_per_core;
    
    /* 
     * Find the thread_id of each cpu
     * e.g., if cpus 2 and 3 are in the same package (i.e. both 
     * have thread_siblings mask = 0xc), then cpu 2 will be "thread 0" 
     * and cpu 3 will be "thread 1"
     */
    for ( i = 0; i < num_cpus; i++ ) {
        flat_threads[i].thread_id = -1;
        int bits_set_so_far = 0;
        j = 0;
        //for (j = 0; j < BITMASK_ARR_LEN; j++ ) {
            unsigned int siblings = flat_threads[i].thread_siblings[j]; 

            //if ( siblings != -1 ) {
                for ( k = 0; k < num_cpus; k++ ) {
                   // if bit 'k' is set 
                   if ( k % (8*sizeof(unsigned int)) == 0 )
                       siblings = flat_threads[i].thread_siblings[j++];
                   if ( siblings & 1 ) {
                       // if it is my bit... 
                       if ( k == i )
                           flat_threads[i].thread_id = bits_set_so_far;
                       bits_set_so_far++;
                   }
                   siblings >>= 1;
                }
            //}
        //}
    }

    /*
     * package, core, and thread info
     */ 
    packinfo_t *package = (packinfo_t*)malloc(num_packages * 
                                              sizeof(packinfo_t));
    if ( !package ) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }
    pi->package = num_packages > 0 ? package : NULL;

    for ( i = 0; i < num_packages; i++ ) {
        core = (coreinfo_t*)malloc(num_cores_per_package *
                                   sizeof(coreinfo_t));

        if ( !core ) {
            fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
            exit(EXIT_FAILURE);
        }
        
        package[i].core = num_cores_per_package > 0 ? core : NULL;

        for ( j = 0; j < num_cores_per_package; j++ ) {
            threadinfo_t **cthread = (threadinfo_t**)malloc(
                                                      num_threads_per_core *
                                                      sizeof(threadinfo_t*));
            if ( !cthread ) {
                fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
                exit(EXIT_FAILURE);
            }

            core[j].thread = num_threads_per_core > 0 ? cthread : NULL;

            for ( k = 0; k < num_threads_per_core; k++ ) {
                for ( l = 0; l < num_cpus; l++ ) {
                    if ( flat_threads[l].pack_id == i && 
                         flat_threads[l].core_id == j &&
                         flat_threads[l].thread_id == k ) {

                        core[j].thread[k] = &flat_threads[l];
                        core[j].sym_core_id = flat_threads[l].sym_core_id;
                        package[i].sym_pack_id = flat_threads[l].sym_pack_id;
                    }
                }
            } // for all threads of core
        } // for all cores of package
    } // for all packages

    /*
     * memnode info
     */ 
    memnodeinfo_t *memnode = (memnodeinfo_t*)malloc(num_memnodes * 
                                                    sizeof(memnodeinfo_t));
    if ( !memnode ) { 
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    pi->memnode = num_memnodes > 0 ? memnode : NULL;

    for ( i = 0; i < num_memnodes; i++ ) {
        sprintf(curr_node, "node%d", i);

        path[0] = '\0';
        strcat(path, base_path);
        strcat(path, "/node/");
        strcat(path, curr_node);
        strcat(path, "/cpumap");
        bitmask_arr_clear(memnode[i].cpumap);
        if ( (fd = open(path, O_RDONLY)) < 0 ) {
            fprintf(stderr, "Could not open %s\n", path);
            //As with every bitmask, commented out
            //memnode[i].cpumap = -1;
        } else {
            br = read(fd, buf, 1024);
            buf[br] = 0;
            trim(buf, ',');
            trim(buf, '\n');
            bitmask_arr_fill_from_hexbuf(memnode[i].cpumap, buf);
            //memnode[i].cpumap = strtol(buf, NULL, 16);
            close(fd);
        }

        path[0] = '\0';
        strcat(path, base_path);
        strcat(path, "/node/");
        strcat(path, curr_node);
        strcat(path, "/meminfo");
        if ( (fd = open(path, O_RDONLY)) < 0 ) {
            fprintf(stderr, "Could not open %s\n", path);
            memnode[i].size = -1;
        } else {
            br = read(fd, buf, 1024);
            buf[br] = 0;
            char *ptr = strstr(buf, "MemTotal:");
            while ( *ptr++ != ':' );
            memnode[i].size = strtol(ptr, NULL, 10) * 1024;
            close(fd);
        }
    } // for all memnodes

    free(sym_pack_ids);
    free(sym_core_ids);

    return pi;
}

/**
 * Reports info
 * @param pi handle to the procmap structure
 */ 
void procmap_report(procmap_t *pi)
{
    assert(pi);

    int i, j, k, l, m, num_cpus = pi->num_cpus;

    fprintf(stdout, "General Info\n");
    fprintf(stdout, "---------------------\n");
    fprintf(stdout, "#Cpus: %d\n",  pi->num_packages *
                                          pi->num_cores_per_package *
                                          pi->num_threads_per_core);

    fprintf(stdout, "#Packages: %d\n", pi->num_packages);
    fprintf(stdout, "#Cores per package: %d\n", pi->num_cores_per_package);
    fprintf(stdout, "#Threads per core: %d\n", pi->num_threads_per_core);
    fprintf(stdout, "\n\n");

    fprintf(stdout, "Flat view\n");
    fprintf(stdout, "---------------------\n");
    for ( i = 0; i < pi->num_cpus; i++ ) {
        int myid = pi->flat_threads[i].cpu_id;
        fprintf(stdout, "Thread %d: Package %d, Core %d ",
                         myid,
                         pi->flat_threads[i].sym_pack_id,
                         pi->flat_threads[i].sym_core_id );

         fprintf(stdout, "[");
         for ( l = 0; l < pi->flat_threads[i].num_caches; l++) {
             char type;
             cacheinfo_t *cache = &pi->flat_threads[i].cache[l];
             
             assert(cache);
             int level = cache->level;
             //int shared_cpu_map = cache->shared_cpu_map;
             type = cache->type[0];
             unsigned int shared_cpu_map;
             int bitmask_chunk = myid / (8*sizeof(unsigned int));
             int offset = myid % (8*sizeof(unsigned int));
             int is_private = ( (1<<offset) == cache->shared_cpu_map[bitmask_chunk] )? 1 : 0;
             for (j = 0; j < BITMASK_ARR_LEN; j++) {
                 shared_cpu_map = cache->shared_cpu_map[j];
                 if ( (j != bitmask_chunk) && (shared_cpu_map != 0) ) {
                     is_private = 0;
                     break;
                 }
                 //if ( (1<<myid) == shared_cpu_map )
                 //    sharers_count++;
             }
             
             if ( is_private ) {
                fprintf(stdout, " L%d%c_pr", level, type);
             } else {
                 fprintf(stdout, " L%d%c_sh(", level, type);
                 j = 0;
                 for ( m = 0; m < num_cpus; m++ ) {
                     if ( m % (8*sizeof(unsigned int)) == 0 )
                         shared_cpu_map = cache->shared_cpu_map[j++];
                     if ( shared_cpu_map & 1 )
                         fprintf(stdout, "%d ", m); 
                     shared_cpu_map >>=1;
                 }
                 fprintf(stdout, ")");
             }
         }
         fprintf(stdout, " ]\n");

    }

    fprintf(stdout, "\n\nHiearachical view\n");
    fprintf(stdout, "---------------------\n");
    for ( i = 0; i < pi->num_packages; i++ ) {
        fprintf(stdout, "Package %d\n", i);

        assert(pi->package);

        for ( j = 0; j < pi->num_cores_per_package; j++ ) {
            fprintf(stdout, "  Core %d\n", j);

            assert(pi->package->core);

            for ( k = 0; k < pi->num_threads_per_core; k++ ) {
                fprintf(stdout, "    Thread %d (system cpu %d) : ",
                        k, pi->package[i].core[j].thread[k]->cpu_id);

                assert(pi->package->core->thread);

                int myid = pi->package[i].core[j].thread[k]->cpu_id;
                
                for ( l = 0; 
                      l < pi->package[i].core[j].thread[k]->num_caches; 
                      l++) {
                    
                    char type[32];
                    cacheinfo_t *cache = 
                        &pi->package[i].core[j].thread[k]->cache[l];
                    
                    assert(cache);

                    int level = cache->level;
                    //int shared_cpu_map = cache->shared_cpu_map;
                    strcpy(type, cache->type);

                    unsigned int shared_cpu_map;
                    int bitmask_chunk = myid / (8*sizeof(unsigned int));
                    int offset = myid % (8*sizeof(unsigned int));
                    int is_private = ( (1<<offset) == cache->shared_cpu_map[bitmask_chunk] )? 1 : 0;
                    int z;
                    for (z = 0; z < BITMASK_ARR_LEN; z++) {
                        shared_cpu_map = cache->shared_cpu_map[z];
                        if ( (z != bitmask_chunk) && (shared_cpu_map != 0) ) {
                            is_private = 0;
                            break;
                        }
                        //if ( (1<<myid) == shared_cpu_map )
                        //    sharers_count++;
                    }

                    if ( is_private ) {
                        fprintf(stdout, "\n              L%d %s (priv)", 
                                        level, type);
                    } else {
                        fprintf(stdout, "\n              L%d %s (shared between ", 
                                        level, type);
                        z = 0;
                        for ( m = 0; m < num_cpus; m++ ) {
                            if ( m % (8*sizeof(unsigned int)) == 0 )
                                shared_cpu_map = cache->shared_cpu_map[z++];
                            if ( shared_cpu_map & 1 )
                                fprintf(stdout, "%d ", m); 
                            shared_cpu_map >>=1;
                        }
                        fprintf(stdout, ")");
                    }
                }
                fprintf(stdout, "\n");
            }

        }

        fprintf(stdout, "\n\n");
    }

    
    assert(pi->memnode);

    fprintf(stdout, "Memory Hiearchy\n");
    fprintf(stdout, "---------------------\n");

    for ( i = 0; i < pi->num_memnodes; i++ ) {
        //int cpumap = pi->memnode[i].cpumap; 
        unsigned int cpumap = pi->memnode[i].cpumap[0];
        fprintf(stdout, "Numa node %d (size %lu) local to cpus ", 
                i, pi->memnode[i].size);
        j = 0;
        for ( m = 0; m < num_cpus; m++ ) {
            if ( m % (8*sizeof(unsigned int)) == 0 )
                cpumap = pi->memnode[i].cpumap[j++];
            if ( cpumap &1 )
                fprintf(stdout, "%d ", m);
            cpumap >>= 1;
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n\n");

    cacheinfo_t *caches = pi->package[0].core[0].thread[0]->cache;
    for ( i = 0; i < _get_num_caches(); i++ ) {
        fprintf(stdout, "Cache %d\n", i);
        fprintf(stdout, "  type: L%d %s\n", 
                caches[i].level, caches[i].type);
        fprintf(stdout, "  size: %lu bytes\n", 
                caches[i].size);
        fprintf(stdout, "  coherency line size: %d bytes\n", 
                        caches[i].coherency_line_size);
        fprintf(stdout, "  number of sets: %d\n", 
                        caches[i].number_of_sets);
        fprintf(stdout, "  ways of associativity: %d\n", 
                        caches[i].ways_of_associativity);
        fprintf(stdout, "\n");
    }
    
}

/**
 * Deallocates memory
 * @param pi handle to the procmap structure
 */  
void procmap_destroy(procmap_t *pi)
{
    int i, j;

    free(pi->memnode);
   
    for ( i = 0; i < pi->num_cpus; i++ ) {
        free(pi->flat_threads[i].cache);
    }
    free(pi->flat_threads);

    for ( i = 0; i < pi->num_packages; i++ ) {
        for ( j = 0; j < pi->num_cores_per_package; j++ ) {
            free(pi->package[i].core[j].thread);
        }
        free(pi->package[i].core);
    }
    free(pi->package);
    free(pi);
}
