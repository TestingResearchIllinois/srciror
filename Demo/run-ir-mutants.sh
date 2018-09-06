#!/bin/bash

# ASSUME SCRIPT IS RUN FROM THE DIRECTORY IT IS UNDER (Demo/)

rm -rf ~/.srciror   # Clean up between runs
mkdir -p ~/.srciror

# Generate all mutants, writes their location into .srciror/
make clean; make CC=$(pwd)/../PythonWrappers/irMutationClang

# Compile each mutant out
for line in $(cat ~/.srciror/bc-mutants/*); do  # Read mutants in SHA identifider for this code (only one under bc-mutants/)
    values=`echo ${line} | cut -f6 -d: | sed 's/,/\n/g'`
    if [ -z "${values}" ]; then # this is a binop
        values=`echo ${line} | cut -f5 -d: | sed 's/,/\n/g'`
    fi
    for v in $(echo ${values}); do  # Iterate through all values that should be replaced with
        mutant_line="max.ll+/home/awshi2/demo/max.c:$(echo ${line} | rev | cut -f2- -d: | rev):${v}"
        echo "${mutant_line}" > ~/.srciror/mutation_request.txt # Mutant is written into mutation_request.txt
        echo "RUNNING MUTANT ${mutant_line}"
        make clean &> /dev/null; make CC=$(pwd)/../PythonWrappers/PythonWrappers/irVanillaClang &> /dev/null
        timeout 10s make test &> /dev/null
        if [[ $? == 0 ]]; then
            echo "SURVIVE"
        else
            echo "KILLED"
        fi
    done
done
