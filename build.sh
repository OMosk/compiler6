#!/bin/bash

set -e
set -x
rm -f compiler
#time clang++ src/all.cpp -o compiler
time clang++ -nodefaultlibs -g -O0 src/all.cpp -o compiler -lc -ldl -pthread -lpthread
time ./compiler
#time valgrind ./compiler
