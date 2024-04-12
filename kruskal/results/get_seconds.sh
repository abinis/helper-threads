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

# UNDER CONSTRUCTION:
# use command-line arguments to construct sed command
# and grab what we want from the input
seconds_patt=.*seconds:\([0-9]*\.[0-9]*\).*
#sed -n -e 's/.*nthreads:\([0-9]*\).*cycles:\([0-9]*\).*seconds:\([0-9]*\.[0-9]*\).*cycles_skipped:\([0-9]*\).*/\1,\2,\3,\4/p' $1 > $1.csv
#sed -n '/^nthreads/h; /cycles_skipped:[0-9]*$/{s/^nthreads/&/; ty; H; :y g; s/.*nthreads:\([0-9]*\).*cycles:\([0-9]*\).*seconds:\([0-9]*\.[0-9]*\).*cycles_skipped:\([0-9]*\).*/\1 \2 \3 \4/p}' $1 > $1.csv
#sed -n '$ {x; s/^$//; t lle; x; H; b print; :lle x; h; b print}; /^nthreads/b print; b skip; :print {x; s/^$//; t end; s/$seconds_patt/\1/p; b end;}; :skip {x;  s/^$//; te; x;  H; b end; :e x;  h;}; :end' $1

sed -n '$ {x; s/^$//;       # last line: exchange pattern and hold space, 
                            # test if hold space empty
           t lle;           # if empty, jump to :lle (skip next part)
           x; H;            # else, exchange back, append pattern (current/last line) to hold
           b print;         # branch to :print
           :lle x; h;       # :lle, same as above, but do not append, instead wipe old contents
                            # of hold and store current line
           b print};        # again, branch to :print
        /^nthreads/b print; # line starts with nthreads; we print the hold space
        b skip;             # on all other lines, we branch to :skip
        :print {x; s/^$//;  # like before, exchange and check for an empty hold space
           t end;           # if empty, jump to :end
           s/.*seconds:\([0-9]*\.[0-9]*\).*/\1/p; # if not empty, substitute contents of hold space
                                                  # (now in the pattern space) with only the things we need, print them   
           s/.*cycles_skipped:\([0-9]*\).*/\1/p;
           b end};
        :skip {x; s/^$//;   # what we do for all other lines: exchange, check for empty hold space
           t end;           # if empty, jump to :end
           x; H};           # if not empty, exchange back, append

           #te; x; H; b end; # if not empty, exchange back, append, branch to :end 
           #:e x; h};        # if empty, exchange back, hold (wipe old contents), end (start cycle anew)
           :end
           ' $1 > $1.csv
