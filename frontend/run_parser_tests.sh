#!/bin/bash

pushd "./parser_tests" > /dev/null
for file in *.c; do
    echo
    echo "Running on file: ${file}"
    echo
    ../minic < "${file}"
done
popd > /dev/null
