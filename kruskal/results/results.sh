#!/bin/bash

if [[ -z $1 ]] || [[ -z $2 ]]
then
   echo "usage: " $0 "<results_file> <output_file>"
   echo "    simple script (sed one-liner actually :P) 
    that parses the input file provided @param <results_file>,
    as produced by running test_mt_kruskal or test_mt_kruskal_array,
    and outputs @param <output_file> a .csv file consisting of 
    three (3) columns:
        numthreads cycles seconds "
   exit
fi

sed -n -e 's/.*nthreads:\([0-9]*\).*cycles:\([0-9]*\).*seconds:\([0-9]*\.[0-9]*\).*/\1,\2,\3/p' $1 > $2.csv
