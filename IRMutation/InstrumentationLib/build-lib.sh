script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
clang -c  LlvmMutateTrace.c -o lib.o
