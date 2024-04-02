#!/bin/bash
mkdir -p build
cd build
gcc ../src/hashing.c ../src/list.c ../src/ini.c ../src/jwms.c -o jwms -Og -g -Wall -Wextra -lconfuse -lbsd
