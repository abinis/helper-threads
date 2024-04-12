#!/bin/bash

#firstcmd="echo this | tee outs"
#$("$firstcmd")
echo 'first cmd'
echo 'second cmd'
echo 'third cmd'

n=0
m=0

iters=15

for i in `seq 1 $((iters*2))`;
do
    flip=$((RANDOM % 1000))
    if [[ (flip -lt 500 && n -lt iters) || (flip -ge 500 && m -ge iters) ]]; then
        echo 'less' # | tee less_out_$((n+10))
        n=$((n+1))
    else
        echo 'more' # | tee more_out_$((m+10))
        m=$((m+1))
    fi
done
