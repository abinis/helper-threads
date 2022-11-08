# helper-threads

#### Note for kruskal/kruskal/test_procmap

In case running `test_procmap` fails with a segmentation fault, it's probably because it wasn't able to open all the required files under /sys/devices/system/cpu/ (as you 'll see in the stderr output).

Try running `ulimit -n` to see what your user quota for simultaneously open files on the system you 're logged in is, and `ulimit -n <number>` to set this to a _sufficiently large number_ :P , e.g. `ulimit -n 65536`.

#### How to use kruskal/results/results.sh

Run your test_mt_kruskal\* experiments, write output to appropriately named files, e.g.

`./test_mt_kruskal ssca2-100000x12017678-interclpr0.5-maxcls200.gr 10 0 | tee ssca2-100000x12017678-10-0` or `./test_mt_kruskal_array random-Gnm-100000x10000000.gr 22 0 | tee array-random-Gnm-100000x10000000-22-0` and so on. Notice the "array" prefix on the output file in the second case, indicating it comes from a `test_mt_kruskal_array` run. We use `tee` to get the output in both the stdout and into our file, a simple redirect will do of course :)

Then, run `./results.sh array-random_Gnm-100000x10000000-22-0` and get a .csv file of the same name, made up of lines of the form nthreads,cycles,seconds.

Needless to say, the script expects your runs to _actually print out_ all of this stuff :P (cycles, seconds etc), like the original `test_mt_kruskal` does; it shouldn't fail otherwise, but you won't get the desired .csv in that case.
