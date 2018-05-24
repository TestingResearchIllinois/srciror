#! /bin/bash

# generate coverage
CURR_DIR=`pwd`
rm -f /tmp/llvm_mutate_trace # remove any existing coverage
rm -rf ~/.srciror # remove logs and results from previous runs
echo "SRCIROR: Instrumenting for Coverage"
SRCIROR_COVINSTRUMENTATION_LIB=$CURR_DIR/../IRMutation/InstrumentationLib/SRCIRORCoverageLib.o SRCIROR_LLVMMutate_LIB=`pwd`/../tools/llvm-build/Release+Asserts/lib/LLVMMutate.so SRCIROR_LLVM_BIN=`pwd`/../tools/llvm-build/Release+Asserts/bin/ python $CURR_DIR/../PythonWrappers/irCoverageClang test.c -o test
# run the executable to collect coverage
./test
echo "the collected coverage is under /tmp/llvm_coverage"


# generate mutation opportunities
echo "SRCIROR: generating mutation opportunities"
SRCIROR_COVINSTRUMENTATION_LIB=$CURR_DIR/../IRMutation/InstrumentationLib/SRCIRORCoverageLib.o SRCIROR_LLVMMutate_LIB=`pwd`/../tools/llvm-build/Release+Asserts/lib/LLVMMutate.so SRCIROR_LLVM_BIN=`pwd`/../tools/llvm-build/Release+Asserts/bin/ python $CURR_DIR/../PythonWrappers/irMutationClang test.c
echo "The generated mutants are under ~/.srciror/bc-mutants/681837891"

# TODO: intersect with coverage
# generate one mutant executable
file_name=`cat ~/.srciror/ir-coverage/hash-map | grep "test.c" | cut -f1 -d:`
mutation=`head -n1 ~/.srciror/bc-mutants/681837891 | cut -f1 -d,`
echo "file name is: $file_name and mutation requested is: $mutation" 
echo "$file_name:$mutation" > ~/.srciror/mutation_request.txt
SRCIROR_COVINSTRUMENTATION_LIB=$CURR_DIR/../IRMutation/InstrumentationLib/SRCIRORCoverageLib.o SRCIROR_LLVMMutate_LIB=`pwd`/../tools/llvm-build/Release+Asserts/lib/LLVMMutate.so SRCIROR_LLVM_BIN=`pwd`/../tools/llvm-build/Release+Asserts/bin/ python $CURR_DIR/../PythonWrappers/irVanillaClang test.c
