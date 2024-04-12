#!/bin/bash

grep -o '\(cycles_ht#[0-9]*:[0-9]* \)*' $1 | sed -n 's/cycles_ht#[0-9]*:\([0-9]*\)/\1/gp' > $1-only-ht.csv
