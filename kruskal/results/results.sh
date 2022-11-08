#!/bin/bash

if [[ -z $1 ]]
then
   echo "usage: " $0 "<results_file>"
   echo "    simple script (sed one-liner actually :P) 
    that parses the input file provided @param <results_file>,
    as produced by test_mt_kruskal* runs,
    and outputs a .csv file of the same name consisting of 
    three (3) columns:
        nthreads,cycles,seconds "
   exit
fi

sed -n -e 's/.*nthreads:\([0-9]*\).*cycles:\([0-9]*\).*seconds:\([0-9]*\.[0-9]*\).*/\1,\2,\3/p' $1 > $1.csv
