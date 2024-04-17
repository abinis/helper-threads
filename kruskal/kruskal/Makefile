INCLUDE_DIR = ../
LIBRARY_DIR = ./

CC = gcc
CFLAGS = -O3 -Wall -g -DPROFILE -D_FORTIFY_SOURCE=0
LDFLAGS =  
LIBS = -lpthread -lm

CFLAGS += -I$(INCLUDE_DIR)

BOOST_DIR = /home/users/abinis/tools/boost_1_83_0/
CPPC = g++
CPPSTD = c++11

TBB = 
#ifeq ($(and $(USE_BOOST_PSORT),$(USE_BOOST_SAMSORT),$(USE_BOOST_BLISORT)),)
#ifeq ($(CMD_MACROS),)
boost = $(shell echo $(CMD_MACROS)|grep -o 'DUSE_BOOST_PSORT\|DUSE_BOOST_SAMSORT\|DUSE_BOOST_BLISORT' )
no_boost_check = $(shell [ -z $(boost) ] && echo true )
ifeq ($(no_boost_check),true)
#ifndef $(boost)
ver = $(shell echo $(CPPSTD)|sed -n 's/c++\([0-9]*\)/\1/p')
res = $(shell [ $(ver) -ge 17 ] && echo true )
ifeq ($(res),true)
TBB = -ltbb
$(info c++ std >= 17, using parallel (TBB) sort)
else
$(info c++ std < 17, using standard library (serial) sort)
endif
else
$(info using c++ boost library sort)
endif
#endif

OMP = -fopenmp

all : test_kruskal test_mt_kruskal test_kruskal_array test_mt_kruskal_array test_mt_kruskal_cas test_mt_kruskal_cas_partial_run test_mt_kruskal_cas_partial_run_overlap test_mt_kruskal_cas_partial_run_col test_mt_kruskal_bitmask test_mt_kruskal_cas_comp test_mt_kruskal_cas_begin

cpp_test_kruskal : kruskal.o cpp_test_kruskal.o edgelist.o adjlist.o union_find.o util.o cpp_psort.o
	$(CC) $(CFLAGS)   kruskal.o cpp_test_kruskal.o edgelist.o \
					  adjlist.o union_find.o util.o cpp_psort.o \
			  		  -o cpp_test_kruskal -std=$(CPPSTD) -lstdc++ $(TBB) -L$(LIBRARY_DIR) $(LIBS)

test_kruskal : kruskal.o test_kruskal.o edgelist.o adjlist.o union_find.o util.o
	$(CC) $(CFLAGS)   kruskal.o test_kruskal.o edgelist.o \
					  adjlist.o union_find.o util.o \
			  		  -o test_kruskal -L$(LIBRARY_DIR) $(LIBS)

test_kruskal_array : kruskal_array.o test_kruskal_array.o edgelist.o adjlist.o union_find_array.o edge_stats.o util.o heap.o
	$(CC) $(CFLAGS)   kruskal_array.o test_kruskal_array.o edgelist.o \
					  adjlist.o union_find_array.o edge_stats.o util.o heap.o \
			  		  -o test_kruskal_array -L$(LIBRARY_DIR) $(LIBS)

test_kruskal_array_sim_fmp : quicksort.o filter_kruskal.o kruskal_array_sim_fmp.o test_kruskal_array.o edgelist.o adjlist.o union_find_array.o edge_stats.o util.o heap.o cpp_psort.o
	$(CC) $(CFLAGS) $(OMP)   quicksort.o filter_kruskal.o kruskal_array_sim_fmp.o test_kruskal_array.o edgelist.o \
					  adjlist.o union_find_array.o edge_stats.o util.o heap.o cpp_psort.o\
			  		  -o test_kruskal_array_sim_fmp -std=$(CPPSTD) -lstdc++ $(TBB) -L$(LIBRARY_DIR) $(LIBS)

test_kruskal_array_sim_filter : filter_kruskal.o kruskal_array_sim_filter.o test_kruskal_array.o edgelist.o adjlist.o union_find_array.o edge_stats.o util.o heap.o
	$(CC) $(CFLAGS)   filter_kruskal.o kruskal_array_sim_filter.o test_kruskal_array.o edgelist.o \
					  adjlist.o union_find_array.o edge_stats.o util.o heap.o \
			  		  -o test_kruskal_array_sim_filter -L$(LIBRARY_DIR) $(LIBS)

test_kruskal_array_sim_qsort : filter_kruskal.o kruskal_array_sim_qsort.o test_kruskal_array.o edgelist.o adjlist.o union_find_array.o edge_stats.o util.o heap.o
	$(CC) $(CFLAGS)   filter_kruskal.o kruskal_array_sim_qsort.o test_kruskal_array.o edgelist.o \
					  adjlist.o union_find_array.o edge_stats.o util.o heap.o \
			  		  -o test_kruskal_array_sim_qsort -L$(LIBRARY_DIR) $(LIBS)

test_kruskal_array_sim_isort : kruskal_array_sim_isort.o test_kruskal_array.o edgelist.o adjlist.o union_find_array.o edge_stats.o util.o heap.o
	$(CC) $(CFLAGS)   kruskal_array_sim_isort.o test_kruskal_array.o edgelist.o \
					  adjlist.o union_find_array.o edge_stats.o util.o heap.o \
			  		  -o test_kruskal_array_sim_isort -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal : kruskal.o mt_kruskal.o test_mt_kruskal.o edgelist.o adjlist.o union_find.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal.o mt_kruskal.o test_mt_kruskal.o edgelist.o \
					  adjlist.o union_find.o util.o processor_map.o \
			  		  -o test_mt_kruskal -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_array : kruskal_array.o mt_kruskal_array.o test_mt_kruskal_array.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_array.o test_mt_kruskal_array.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_array -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_cas : kruskal_array.o mt_kruskal_cas.o test_mt_kruskal_cas.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_cas.o test_mt_kruskal_cas.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_cas -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_cas_partial_run : kruskal_array.o mt_kruskal_cas_partial_run.o test_mt_kruskal_cas_partial_run.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_cas_partial_run.o test_mt_kruskal_cas_partial_run.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_cas_partial_run -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_cas_partial_run_overlap : kruskal_array.o mt_kruskal_cas_partial_run.o test_mt_kruskal_cas_partial_run_overlap.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_cas_partial_run.o test_mt_kruskal_cas_partial_run_overlap.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_cas_partial_run_overlap -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_cas_partial_run_col : kruskal_array_col.o mt_kruskal_cas_partial_run_col.o test_mt_kruskal_cas_partial_run_col.o edgelist_col.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array_col.o mt_kruskal_cas_partial_run_col.o test_mt_kruskal_cas_partial_run_col.o edgelist_col.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_cas_partial_run_col -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_bitmask : kruskal_array.o mt_kruskal_bitmask.o test_mt_kruskal_bitmask.o edge_color_bitmask.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_bitmask.o test_mt_kruskal_bitmask.o edge_color_bitmask.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_bitmask -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_cas_comp : kruskal_array.o mt_kruskal_cas_comp.o test_mt_kruskal_cas_comp.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o heap.o cpp_psort.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_cas_comp.o test_mt_kruskal_cas_comp.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o heap.o cpp_psort.o \
			  		  -o test_mt_kruskal_cas_comp -std=$(CPPSTD) -lstdc++ $(TBB) -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal_cas_begin : kruskal_array.o mt_kruskal_cas_begin.o test_mt_kruskal_cas_partial_run.o edgelist.o adjlist.o union_find_array.o util.o processor_map.o
	$(CC) $(CFLAGS)   kruskal_array.o mt_kruskal_cas_begin.o test_mt_kruskal_cas_partial_run.o edgelist.o \
					  adjlist.o union_find_array.o util.o processor_map.o \
			  		  -o test_mt_kruskal_cas_begin -L$(LIBRARY_DIR) $(LIBS)

test_procmap : processor_map.o util.o test_procmap.o 
	$(CC) $(CFLAGS)   processor_map.o util.o test_procmap.o \
					  -o test_procmap -L$(LIBRARY_DIR) $(LIBS)

#test_mt_kruskal.o : test_mt_kruskal.c
#	$(CC) $(LDFLAGS)  -g -DPROCMAP_H="processor_map/processor_map.h" -c test_mt_kruskal.c -o test_mt_kruskal.o
#
#test_mt_kruskal_new.o : test_mt_kruskal.c
#	$(CC) $(LDFLAGS)  -g -DPROCMAP_H="processor_map/processor_map_new.h" -c test_mt_kruskal.c -o test_mt_kruskal_new.o
#

edgelist.o : ../graph/edgelist.c
	$(CC) $(CFLAGS)  -c ../graph/edgelist.c 

edgelist_col.o : ../graph/edgelist_col.c
	$(CC) $(CFLAGS)  -c ../graph/edgelist_col.c 

adjlist.o : ../graph/adjlist.c
	$(CC) $(CFLAGS)  -c ../graph/adjlist.c

processor_map.o : ../processor_map/processor_map.c
	$(CC) $(CFLAGS)  -c ../processor_map/processor_map.c

union_find.o : ../disjoint_sets/union_find.c
	$(CC) $(CFLAGS)  -c ../disjoint_sets/union_find.c

union_find_array.o : ../disjoint_sets/union_find_array.c
	$(CC) $(CFLAGS)  -c ../disjoint_sets/union_find_array.c

util.o : ../util/util.c
	$(CC) $(CFLAGS)  -c ../util/util.c

heap.o : ../heap/heap.c
	$(CC) $(CFLAGS)  -c ../heap/heap.c

kruskal_array_sim_qsort.o : kruskal_array.c
	$(CC) $(CFLAGS)  -c -DUSE_QSORT kruskal_array.c -o kruskal_array_sim_qsort.o

kruskal_array_sim_isort.o : kruskal_array.c
	$(CC) $(CFLAGS)  -c -DUSE_ISORT kruskal_array.c -o kruskal_array_sim_isort.o

filter_kruskal.o : ../filter-kruskal/filter_kruskal.c
	$(CC) $(CFLAGS) $(OMP) $(CMD_MACROS)  -c ../filter-kruskal/filter_kruskal.c

kruskal_array_sim_filter.o : kruskal_array.c
	$(CC) $(CFLAGS) $(CMD_MACROS)  -c -DUSE_QSORT -DUSE_FILTER_KRUSKAL kruskal_array.c -o kruskal_array_sim_filter.o

kruskal_array_sim_fmp.o : kruskal_array.c
	$(CC) $(CFLAGS) $(CMD_MACROS) $(OMP)  -c -DUSE_QSORT -DUSE_FILTER_KRUSKAL -DMARK_AND_PUSH kruskal_array.c -o kruskal_array_sim_fmp.o

test_kruskal_array.o : test_kruskal_array.c
	$(CC) $(CFLAGS) $(CMD_MACROS) $(OMP)  -c test_kruskal_array.c -o test_kruskal_array.o

#cpp_psort.o : ../cpp_functions/cpp_psort.cpp
#	$(CPPC) $(CFLAGS) -I$(BOOST_DIR) $(CMD_MACROS)  -c ../cpp_functions/cpp_psort.cpp -std=$(CPPSTD)

quicksort.o : ../dgal-quicksort/quicksort.c
	$(CC) $(CFLAGS) $(OMP)  -c $<

cpp_psort.o : ../cpp_functions/cpp_psort.cpp
	$(CPPC) $(CFLAGS) -I$(BOOST_DIR) -I$(INCLUDE_DIR) $(CMD_MACROS) -c ../cpp_functions/cpp_psort.cpp -std=$(CPPSTD)

%.o : %.c 
	$(CC) $(CFLAGS) $(CMD_MACROS) -c $<


clean :
	rm -f test_kruskal test_mt_kruskal test_procmap test_kruskal_array test_mt_kruskal_array test_mt_kruskal_cas test_mt_kruskal_cas_partial_run test_mt_kruskal_cas_partial_run_overlap *.o 
