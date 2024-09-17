#!/bin/bash
valgrind -s --leak-check=full --track-origins=yes ./build/debug/jwms --all
