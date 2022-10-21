/**
 * @file
 * Type definitions and function declarations for various 
 * components of the system processor hierarchy.
 */ 

#ifndef PROCESSOR_MAP_H_
#define PROCESSOR_MAP_H_

#define BITMASK_ARR_LEN 8

/**
 * Cache info 
 */ 
typedef struct {
    int coherency_line_size;
    int level;
    int number_of_sets;
    int physical_line_partition;
    unsigned int shared_cpu_map[BITMASK_ARR_LEN]; //!< map of sharers
    //char shared_cpu_map[32];
    unsigned long size; //!< size in bytes
    char type[32];
    int ways_of_associativity;
} cacheinfo_t;

/**
 * Hardware thread info (logical processor).
 * The HW thread is the lowest level in processor hierarchy 
 * and has a unique, system-wide cpu_id
 */ 
typedef struct {
    //! Unique cpu number (as "seen" be affinity syscalls)
    int cpu_id; 

    //! Relative thread id (to distinguish threads within 
    //! the same core).
    //! Thread id's are unique only within the same core. 
    int thread_id;     

    //! The symbolic id of the containing core, as reported 
    //! by the sys filesystem.
    //! Symbolic id's may not be contiguous numbers w.r.t. 
    //! the ids of the rest core siblings.
    //! Core id's are unique only within the same package.
    int sym_core_id;

    //! The consequtive id of the containing core.
    //! This id is contiguous w.r.t. the ids of the rest
    //! core siblings
    int core_id;       

    //! The symbolic id of the containing package, as reported 
    //! by the sys filesystem.
    //! Symbolic id's of packages may not be contiguous numbers
    //! (systemwide-ly) 
    int sym_pack_id; 

    //! The consequtive id of he containing packages.
    //! This id is contiguous w.r.t. the ids of the rest
    //! packages in the system
    int pack_id;

    //! Bitmap in hex format that shows the core siblings of the 
    //! current cpu. All cpus belonging to the same package (i.e. 
    //! are core siblings) have their corresponding bits (indexed by 
    //! their cpu_id) set in the mask

    //! Under construction: for systems with *many* cores (e.g broady),
    //! int type does not suffice (not even long int or maybe long long
    //! int I would argue). An int array should be more appropriate.
    //! In each position, we store 8 bytes (characters) of the parsed 
    //! hex string, interpreted as an int (4 bytes).
    //! The same is true for any other mask, e.g. the thread_siblings mask
    //! just below, and the shared_cpu_maps utilised by the caches.
    unsigned int core_siblings[BITMASK_ARR_LEN];
    //char core_siblings[32]

    //! Bitmap in hex format that shows the thread siblings of the 
    //! current cpu, i.e. shows which other cpu_ids are threads 
    //! belonging to the same package 
    unsigned int thread_siblings[BITMASK_ARR_LEN];
    //char thread_siblings[32];

    //! Number of different caches (shared or not) that this
    //! thread sees.
    int num_caches;            

    //! Array of caches
    cacheinfo_t *cache;        

} threadinfo_t;

/**
 * Core info.
 * A core contains one or more hardware threads.
 */
typedef struct {
    int sym_core_id; //!< same as in threadinfo_t 
    threadinfo_t **thread; //!< array of pointers to hw threads
} coreinfo_t;

/**
 * Physical package (chip) info.
 * A physical package contains one or more cores.
 */ 
typedef struct {
    int sym_pack_id; //!< same as in threadinfo_t 
    coreinfo_t *core;
} packinfo_t;

/**
 * Memory node info
 */
typedef struct {
    //! Bitmap that shows which cpu_ids are local to 
    //! this memory node
    unsigned int cpumap[BITMASK_ARR_LEN];     

    //! Size in bytes of this memory node
    unsigned long size;
} memnodeinfo_t;

/**
 * Processor hierarchy info.
 * The hierarchy starts with one or more 
 * physical packages.
 */ 
typedef struct {
    //! Total number of cpus (hardware threads)
    int num_cpus;

    //! Total number of physical packages
    int num_packages;

    //! Total number of memory nodes
    int num_memnodes;

    int num_cores_per_package;
    int num_threads_per_core;
    int num_caches_per_thread;
    
    //! Flat view of hw threads
    threadinfo_t *flat_threads;

    //! "Root" to hierarchical view of hw threads
    packinfo_t *package;

    //! Memory nodes of the system
    memnodeinfo_t *memnode;
} procmap_t;

procmap_t* procmap_init(void); 
void procmap_report(procmap_t *pi);
void procmap_destroy(procmap_t *pi);

#endif
