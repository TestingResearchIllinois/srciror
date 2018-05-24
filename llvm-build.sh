#! /bin/bash -e

# ================= Main Body ===================== #
ZIPS_DIR=zips
TOOLS_DIR=tools
LLVM_DIR=llvm
LLVM_BUILD=llvm-build
mkdir -p $ZIPS_DIR
mkdir -p $TOOLS_DIR
# get the sources of clang, compiler-rt, and llvm
wget -N http://llvm.org/releases/3.8.1/llvm-3.8.1.src.tar.xz -P $ZIPS_DIR/
wget -N http://llvm.org/releases/3.8.1/compiler-rt-3.8.1.src.tar.xz -P $ZIPS_DIR/
wget -N http://llvm.org/releases/3.8.1/cfe-3.8.1.src.tar.xz -P $ZIPS_DIR/
        
# unzip
tar xfJ  $ZIPS_DIR/llvm-3.8.1.src.tar.xz
tar xfJ  $ZIPS_DIR/compiler-rt-3.8.1.src.tar.xz
tar xfJ  $ZIPS_DIR/cfe-3.8.1.src.tar.xz

# relocate the sources
rm -rf $LLVM_DIR
mv llvm-3.8.1.src $LLVM_DIR
mv compiler-rt-3.8.1.src $LLVM_DIR/projects/compiler-rt
mv cfe-3.8.1.src $LLVM_DIR/tools/clang

# build llvm
rm -rf  $LLVM_BUILD
mkdir $LLVM_BUILD
cd $LLVM_BUILD
export C_INCLUDE_PATH= 
export CPLUS_INCLUDE_PATH= 
../llvm/configure
# cmake  -G "Unix Makefiles" ../llvm
make -j4
