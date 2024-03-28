#!/bin/bash
mkdir -p build
cd build
gcc ../hashing.c ../list.c ../jwms.c -o jwms -O0 -g -Wall -Wextra -lconfuse -lbsd
