#!/bin/bash
valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./build/debug/jwm-helper -a
