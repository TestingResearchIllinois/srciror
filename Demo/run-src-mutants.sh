#!/bin/bash

# ASSUME SCRIPT IS RUN FROM THE DIRECTORY IT IS UNDER (Demo/)

mkdir -p ~/.srciror
rm -rf ~/.srciror/src-mutants/  # Clean up between runs

# Write out coverage for src, to speed things up
echo "$(pwd)/max.c:5,6,7,8,9,10,13" > ~/.srciror/coverage

# Make mutants with the Python wrapper and move into place
make clean; make CC=$(pwd)/../PythonWrappers/mutationClang 
mkdir -p ~/.srciror/src-mutants/
mv *.mut.*.c ~/.srciror/src-mutants/

# Save original file, run tests on on each mutant
cp max.c max.c.orig
for m in $(ls ~/.srciror/src-mutants/); do
    cp ~/.srciror/src-mutants/${m} max.c
    echo "RUNNING MUTANT ${m}"
    make clean &> /dev/null; make &> /dev/null
    timeout 10s make test &> /dev/null
    if [[ $? == 0 ]]; then
        echo "SURVIVE"
    else
        echo "KILLED"
    fi
done
cp max.c.orig max.c # Return original file
