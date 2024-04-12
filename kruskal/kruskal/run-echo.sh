#!/bin/bash

#rmat=./test_mt_kruskal_cas_comp /s/graphs/1M/rmat-1000000x10000000-a0.45b0.15c0.15d0.25.gr 8 0 1 100 | tee partial-rmat-1x10-8-100_$i

n=0
m=0

maxthr=8
datapts=100

iters=10

for i in `seq 1 $((iters*2))`
do
    flip=$((RANDOM % 1000))
    if [[ (flip -lt 500 && n -lt iters) || (flip -ge 500 && m -ge iters) ]]; then
        echo './test_mt_kruskal_cas_comp /s/graphs/1M/rmat-1000000x10000000-a0.45b0.15c0.15d0.25.gr $maxthr 0 1 $datapts  | tee partial-rmat-1x10-$maxthr-${datapts}_$n'
        n=$((n+1))
    else
        echo './test_mt_kruskal_cas_comp /s/graphs/1M/ssca2-1000000x11820688-interclpr0.5-maxcls20.gr $maxthr 0 1 $datapts  | tee partial-ssca2-1x11.8-$maxthr-${datapts}_$m'
        m=$((m+1))
    fi
    #sleep $((RANDOM % 300 + 300))
done
