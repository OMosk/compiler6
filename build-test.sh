#!/bin/bash

set -e
set -x
rm -f compiler
#time clang++ src/all.cpp -o compiler
time clang++ -nodefaultlibs -g -O0 src/test_main.cpp -o tests -lc -ldl -pthread -lpthread
