#!/bin/bash

# ASSUME SCRIPT IS RUN FROM SAME DIRECTORY (Demo/)

# Assume the llvm-build/ is in location above Demo/, as defined by llvm-build.sh script
export SRCIROR_LLVMMutate_LIB=$(pwd)/../llvm-build/Release+Asserts/lib/LLVMMutate.so
export SRCIROR_SRC_MUTATOR=$(pwd)/../SRCMutation/build/mutator
export SRCIROR_LLVM_BIN=$(pwd)/../llvm-build/Release+Asserts/bin/
export SRCIROR_LLVM_INCLUDES=$(pwd)/../llvm-build/Release+Asserts/lib/clang/3.8.0/include/
