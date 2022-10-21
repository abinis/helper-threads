INCLUDE_DIR = ../
LIBRARY_DIR = ./

CC = gcc
CFLAGS = -O3 -Wall -DPROFILE -D_FORTIFY_SOURCE=0
LDFLAGS =  
LIBS = -lpthread 

CFLAGS += -I$(INCLUDE_DIR)

all : test_kruskal test_mt_kruskal

test_kruskal : kruskal.o test_kruskal.o edgelist.o adjlist.o union_find.o util.o
	$(CC) $(LDFLAGS)  -g kruskal.o test_kruskal.o edgelist.o \
					  adjlist.o union_find.o util.o \
			  		  -o test_kruskal -L$(LIBRARY_DIR) $(LIBS)

test_mt_kruskal : kruskal.o mt_kruskal.o test_mt_kruskal.o edgelist.o adjlist.o union_find.o util.o processor_map.o
	$(CC) $(LDFLAGS)  -g kruskal.o mt_kruskal.o test_mt_kruskal.o edgelist.o \
					  adjlist.o union_find.o util.o processor_map.o \
			  		  -o test_mt_kruskal -L$(LIBRARY_DIR) $(LIBS)

test_procmap : processor_map.o util.o test_procmap.o 
	$(CC) $(LDFLAGS)  -g processor_map.o util.o test_procmap.o \
					  -o test_procmap -L$(LIBRARY_DIR) $(LIBS)

#test_mt_kruskal.o : test_mt_kruskal.c
#	$(CC) $(LDFLAGS)  -g -DPROCMAP_H="processor_map/processor_map.h" -c test_mt_kruskal.c -o test_mt_kruskal.o
#
#test_mt_kruskal_new.o : test_mt_kruskal.c
#	$(CC) $(LDFLAGS)  -g -DPROCMAP_H="processor_map/processor_map_new.h" -c test_mt_kruskal.c -o test_mt_kruskal_new.o
#

edgelist.o : ../graph/edgelist.c
	$(CC) $(CFLAGS) -g -c ../graph/edgelist.c 

adjlist.o : ../graph/adjlist.c
	$(CC) $(CFLAGS) -g -c ../graph/adjlist.c

processor_map.o : ../processor_map/processor_map.c
	$(CC) $(CFLAGS) -g -c ../processor_map/processor_map.c

union_find.o : ../disjoint_sets/union_find.c
	$(CC) $(CFLAGS) -g -c ../disjoint_sets/union_find.c

util.o : ../util/util.c
	$(CC) $(CFLAGS) -g -c ../util/util.c

%.o : %.c 
	$(CC) $(CFLAGS) -g -c $<


clean :
	rm -f test_kruskal test_mt_kruskal test_procmap *.o
