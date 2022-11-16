#!/bin/bash

if [[ -z $1 ]]
then
   echo "usage: " $0 "<results_file>"
   echo "    simple script (sed one-liner actually :P) 
    that parses the input file provided @param <results_file>,
    as produced by test_mt_kruskal* runs,
    and outputs a .csv file with columns of whatever parameters
    we might want, e.g.
        nthreads,cycles,seconds,cycles_skipped and so on
    TODO: use command-line arguments to choose what to output"
   exit
fi

#sed -n -e 's/.*nthreads:\([0-9]*\).*cycles:\([0-9]*\).*seconds:\([0-9]*\.[0-9]*\).*cycles_skipped:\([0-9]*\).*/\1,\2,\3,\4/p' $1 > $1.csv
sed -n '/^nthreads/h; /cycles_skipped:[0-9]*$/{s/^nthreads/&/; ty; H; :y g; s/.*nthreads:\([0-9]*\).*cycles:\([0-9]*\).*seconds:\([0-9]*\.[0-9]*\).*cycles_skipped:\([0-9]*\).*/\1 \2 \3 \4/p}' $1 > $1.csv
