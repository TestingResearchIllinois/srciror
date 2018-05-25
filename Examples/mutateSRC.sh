#!/bin/bash

# generate all src mutants (assuming all lines are covered)
CURR_DIR=$( cd $( dirname $0 ) && pwd )

rm -rf ~/.srciror
mkdir ~/.srciror
echo "$CURR_DIR/test.c:3,4,5,6,7,8,9" > ~/.srciror/coverage

SRCIROR_LLVM_BIN=$CURR_DIR/../tools/llvm-build/Release+Asserts/bin
SRCIROR_LLVM_INCLUDES=$CURR_DIR/../tools/llvm-build/Release+Asserts/lib/clang/3.8.0/include
SRCIROR_SRC_MUTATOR=$CURR_DIR/../SRCMutation/build/mutator
python $CURR_DIR/../PythonWrappers/mutationClang $CURR_DIR/test.c -o test
