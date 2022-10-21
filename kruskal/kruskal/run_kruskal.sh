#!/bin/bash

# $GRAPH_SIZE: 10K, 100K, 1M, 10M
# is env variable and must be set from where qsub is called from

cd $HOME/trac/kruskal/

# get max number of processors
proc_num=$(cat /proc/cpuinfo | grep "processor" | wc -l)

outfile=$(hostname)_output_${GRAPH_SIZE}graphs.txt
rm -f $outfile

for g in $(ls /local/graphs/$GRAPH_SIZE/*.gr)
do
    ./test_mt_kruskal $g $proc_num 0 >> $outfile
    ./test_mt_kruskal $g $proc_num 1 >> $outfile
done
