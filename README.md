# helper-threads

#### Note for kruskal/kruskal/test_procmap

In case running `test_procmap` fails with a segmentation fault, it's probably because it wasn't able to open all the required files under /sys/devices/system/cpu/ (as you 'll see in the stderr output).

Try running `ulimit -n` to see what your user quota for simultaneously open files on the system you 're logged in is, and `ulimit -n <number>` to set this to a _sufficiently large number_ :P , e.g. `ulimit -n 65536`.
