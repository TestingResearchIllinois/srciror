CURR_DIR := $(shell pwd)
LLVM_SRC_PATH := $(CURR_DIR)/../tools/llvm
LLVM_BUILD_PATH := $(CURR_DIR)/../tools/llvm-build
LLVM_BIN_PATH := $(LLVM_BUILD_PATH)/Release+Asserts/bin

CXX := g++
CXXFLAGS := -fno-rtti -O0 -g
PLUGIN_CXXFLAGS := -fpic

LLVM_CXXFLAGS := `$(LLVM_BIN_PATH)/llvm-config --cxxflags`
LLVM_LDFLAGS := `$(LLVM_BIN_PATH)/llvm-config --ldflags --libs --system-libs`
LLVM_LDFLAGS_NOLIBS := `$(LLVM_BIN_PATH)/llvm-config --ldflags`
PLUGIN_LDFLAGS := -shared

# These are required when compiling vs. a source distribution of Clang. For
# binary distributions llvm-config --cxxflags gives the right path.
CLANG_INCLUDES := \
	-I$(LLVM_SRC_PATH)/include \
	-I$(LLVM_BUILD_PATH)/include \
	-I$(LLVM_SRC_PATH)/tools/clang/include \
	-I$(LLVM_BUILD_PATH)/tools/clang/include

# List of Clang libraries to link. The proper -L will be provided by the
# call to llvm-config
CLANG_LIBS := \
	-Wl,--start-group \
	-lclangAST \
	-lclangAnalysis \
	-lclangBasic \
	-lclangDriver \
	-lclangEdit \
	-lclangFrontend \
	-lclangFrontendTool \
	-lclangLex \
	-lclangParse \
	-lclangSema \
	-lclangEdit \
	-lclangASTMatchers \
	-lclangRewrite \
	-lclangRewriteFrontend \
	-lclangStaticAnalyzerFrontend \
	-lclangStaticAnalyzerCheckers \
	-lclangStaticAnalyzerCore \
	-lclangSerialization \
	-lclangToolingCore \
	-lclangTooling \
	-Wl,--end-group

# Internal paths in this project: where to find sources, and where to put
# build artifacts.
SRC_CLANG_DIR := src
BUILDDIR := build

.PHONY: all
all: make_builddir \
	emit_build_config \
	$(BUILDDIR)/mutator \


.PHONY: emit_build_config
emit_build_config: make_builddir
	@echo $(LLVM_BIN_PATH) > $(BUILDDIR)/_build_config

.PHONY: make_builddir
make_builddir:
	@test -d $(BUILDDIR) || mkdir $(BUILDDIR)

$(BUILDDIR)/mutator: $(SRC_CLANG_DIR)/mutator.cpp
	$(CXX) $(CXXFLAGS) $(LLVM_CXXFLAGS) $(CLANG_INCLUDES) $^ \
		$(CLANG_LIBS) $(LLVM_LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/* *.dot test/*.pyc test/__pycache__
