#!/bin/bash

#rmat=./test_mt_kruskal_cas_comp /s/graphs/1M/rmat-1000000x10000000-a0.45b0.15c0.15d0.25.gr 8 0 1 100 | tee partial-rmat-1x10-8-100_$i

for i in `seq 1 10`
do
    ./test_mt_kruskal_cas_comp /s/graphs/1M/rmat-1000000x10000000-a0.45b0.15c0.15d0.25.gr 22 0 1 100  | tee partial-rmat-1x10-22-100_$i
    sleep $((RANDOM % 300 + 300))
done
