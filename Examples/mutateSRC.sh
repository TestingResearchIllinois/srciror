#!/bin/bash

# generate all src mutants (assuming all lines are covered)
CURR_DIR=`pwd`

rm -rf ~/.srciror
mkdir ~/.srciror
echo "`pwd`/test.c:3,4,5,6,7,8,9" > ~/.srciror/coverage

SRCIROR_LLVM_BIN=`pwd`/../tools/llvm-build/Release+Asserts/bin SRCIROR_LLVM_INCLUDES=`pwd`/../tools/llvm-build/Release+Asserts/lib/clang/3.8.0/include SRCIROR_SRC_MUTATOR=`pwd`/../SRCMutation/build/mutator python $CURR_DIR/../PythonWrappers/mutationClang `pwd`/test.c -o test

