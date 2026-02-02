#!/bin/bash

pushd "./semantic_analysis_tests" > /dev/null 
for file in *.c; do
    if [[ "${file}" == "main.c" ]]; then
        continue
    fi
    echo "Result for the file: ${file} is: "
    ../minic < "${file}"
done
popd > /dev/null
echo "All the test files completed running"
