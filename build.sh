#!/bin/bash
mkdir -p build
cd build
gcc ../jwms.c -o jwms -O0 -g -Wall -Wextra -lconfuse -lbsd