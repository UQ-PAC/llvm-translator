# llvm-translator

An ad-hoc collection of pieces to compare (semantically) the output of several AArch64 lifters, namely asl-interpreter, RetDec (via Capstone), and Remill. 
asl-interpreter is used as the baseline for comparison. 

Requires:
- LLVM 14
- https://github.com/UQ-PAC/aslp/tree/partial_eval/, beside this directory and with folder name _asl-interpreter_.
- https://github.com/AliveToolkit/alive2, beside this directory and with translation validation ([see README](https://github.com/AliveToolkit/alive2#building-and-running-translation-validation)). Commit [`bc51b72c`](https://github.com/AliveToolkit/alive2/commit/bc51b72cf5773967fd29155f1ffb251df4d5e94e) with cherry-pick [`9a7504a9`](https://github.com/AliveToolkit/alive2/commit/9a7504a99972e2c613deacaa8a4f1798829d2ff2) and LLVM 15 from [here](https://github.com/katrinafyi/pac-environment/releases/tag/llvm).
- https://github.com/avast/retdec, beside and built with `cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/prefix -DRETDEC_DEV_TOOLS=1 -DCMAKE_CXX_FLAGS='-include cstdint' -DCMAKE_CXX_FLAGS_RELEASE='-include cstdint'`. Tested with RetDec [v5.0](https://github.com/avast/retdec/tree/v5.0). 
- https://github.com/lifting-bits/remill, as "remill" Docker container.

Usage:
- `tools/env.sh` will set up environment variables for later use. Run this first to check the dependencies can be found correctly.
- `tools/glue.sh 2100028b` performs the comparison on the opcode 2100028b. Output is printed to stdout and supplementary logs are written to /tmp.
- `tools/bulk.sh logs_dir` performs the comparison on many opcodes, calling glue.sh for each one. 
  - Progress is printed to stdout and comparison results (i.e. from glue.sh) are written to subfolders of logs_dir.
  - Opcodes are sourced from ../asl-interpreter/tests/coverage/\*, which has lists of opcodes liftable by the asl-interpreter.
- `tools/log_parser.py logs_dir out.csv` parses the log directory logs_dir which should contain the output of bulk.sh. Results are tabulated for further analysis.

Components:
- asl-translator/ contains an OCaml dune project which translates asl-interpreter's reduced ASL into LLVM IR.
  ```bash
  cd asl-translator
  eval $(opam env)  # make sure to use non-system ocaml compiler
  opam pin ../../asl-interpreter  # path to asl-interpreter repository
  opam install --deps-only ./asl-translator.opam  # also install LLVM 14 through system packages
  dune build
  ```
  Then, dump ASL from asl-interpreter with `:dump A64 0x8b020021 /tmp/sem.aslb`, then run asl-translator with `dune exec test /tmp/sem.aslb`.
- This directory (llvm-translator) is a C++ project which unifies the LLVM IR from each lifter. 
  ```bash
  cmake -B build .
  cmake --build build
  ./go rem /tmp/remill_out.ll  # also supports 'cap' and 'asl'
  ```
- tools/post.sh is used to post-process and simplify the output of llvm-translator before passing to alive. It calls opt and runs a given list of passes. 
- Further, Alive2 requires source/target to have the same set of global variables. llvm-translator supports `./go vars /tmp/cap.ll /tmp/rem.ll /tmp/asl.ll` which will union all variables mentioned by each lifter and insert them into the others.
- in/ and out/ contain old snapshots of LLVM code, as an example of the different LLVM IR styles from each lifter. in/ is directly from the lifter in question, and out/ is after (an old version of) llvm-translator.
