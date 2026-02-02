#!/bin/bash

echo "Running the good input"
for file in p[1-5].c; do
    echo "Running on file: ${file}"
    ./minic < "${file}"
done

echo "Now running the bad input"
./minic < p_bad.c
