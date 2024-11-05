#!/bin/bash
astyle --style=allman --recursive src/\*.c

for i in src/*.c.orig; do
    [ -f "$i" ] || break
    new_name="${i%.orig}"
    mv $i $new_name 
done
