#ifndef EDGE_STATS_H__
#define EDGE_STATS_H__

#define NUM_DATAPOINTS 100

void print_edge_stats(unsigned int len, int *edge_membership,
                      int total_msf_edges, int total_cycle_edges);

void print_gap_distribution(unsigned int len, int *edge_membership);

#endif
