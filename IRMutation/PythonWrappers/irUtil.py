import os, sys

CWD = os.path.dirname(os.path.abspath(__file__))
from bashUtil import executeCommand

def getSummaryDir():
    # make the summary directory if does not exist
    # also makes the ir-coverage/ directory
    summaryDir = os.path.join(os.getenv("HOME"), ".optimute")
    if not os.path.exists(summaryDir):
        os.makedirs(summaryDir)
    return summaryDir


def findSrcIndices(arg_list, ext):
    source_indices = [i for i, word in enumerate(arg_list) if word.endswith(ext)]
    return source_indices


def run(task, taskFunction):
    llvm_bin_dir = os.environ["SRCIROR_LLVM_BIN"]
    clang = os.path.join(llvm_bin_dir, 'clang')
    opt = os.path.join(llvm_bin_dir, 'opt')
    mutation_lib = os.environ["SRCIROR_LLVMMutate_LIB"] # path to llvm_build/Release+Asserts./lib/LLVMMutate.so
    if task == "coverage":
        our_lib = os.environ["SRCIROR_COVINSTRUMENTATION_LIB"] # path to IRMutation/InstrumentationLib/lib.o
    else:
        our_lib = ""
    coverageDir = os.path.join(getSummaryDir(), "ir-coverage")  # guarantees directory is made
    if not os.path.exists(coverageDir):
        os.makedirs(coverageDir)
    log_file = os.path.join(coverageDir, "hash-map")
    opt_command = opt + " -load " + mutation_lib
    args = sys.argv[1:]

    if '-fstack-usage' in args: # TODO: What is -fstack-usage?
        args.remove('-fstack-usage')
    compiler = clang
    print('logging compile flags: ' + ' '.join(args))

    # if the build system is checking for the version flags, we don't mess it up, just delegate to the compiler
    if "--version" in args or "-V" in args:
        out, err, _ = executeCommand([compiler] + args, True)
        return 1

    ###### instrument ######
    # if the command contains a flag that prevents linking, then it's
    # not generating an executable, then we should be able to generate
    # bitcode and instrument it for the task
    # 1. find if command is compiling some source .c file
    src_indices = findSrcIndices(args, ".c")
    if not src_indices: # there is no source code to instrument, so we want to try and link, no changes
        print('Looking to link: ' + str([compiler, our_lib] + args))
        out, err, _ = executeCommand([compiler, our_lib] + args, True)
        print(str(out))
        print(str(err))
        return 1    # TODO: Why do we return 1 instead of 0?

    # if there are multiple src files, only care about the first
    src_index = src_indices[0]
    src_file = args[src_indices[0]]

    # 2. see if there is a specified -o; if exists, use it to determine name of intermediate .ll file
    new_args = list(args)
    try:
        dash_o_index = new_args.index('-o')
        out_name = new_args[dash_o_index + 1]
        print("we got the dash o index")
        print("the out name is : " + out_name)
    except:
        out_name = src_file # if does not exist, use the src file name as the base
        print("we did not find dash o index, so we are using src: " + src_file)
        dash_o_index = len(new_args)    # filling up the args with some empty placeholders for upcoming update
        new_args += ["-o", ""]
    bitcode_file = os.path.dirname(out_name) + os.path.basename(out_name).split(".")[0] + ".ll" # expect only one . in the name
    print("the bitcode file name is: " + bitcode_file)

    # 3. put flags to generate bitcode
    new_args[dash_o_index + 1] = bitcode_file
    print("we are emitting llvm bitcode: " + " ".join([clang, '-S', '-emit-llvm'] + new_args))
    executeCommand([clang, '-S', '-emit-llvm'] + new_args)

    # 4. instrument the bitcode for the specified task
    taskFunction(bitcode_file, src_file, opt_command, log_file)

    # 5. compile the .ll into the real output so compilation can proceed peacefully
    #    replace the input C src file with the bitcode file we determined
    compiling_args = list(args)
    compiling_args[src_index] = bitcode_file
    command = [clang] + compiling_args + [our_lib]
    print("we are generating the output from the .ll with the command" + " ".join(command))
    out, err, _ = executeCommand(command, True)
    print(str(out))
    print(str(err))
    return 1


