# llvm-translator

An ad-hoc collection of pieces to compare (semantically) the output of several AArch64 lifters, namely asl-interpreter, RetDec (via Capstone), and Remill. 
asl-interpreter is used as the baseline for comparison. 

Requires:
- LLVM 14
- https://github.com/UQ-PAC/asl-interpreter/tree/partial_eval/, beside this directory.
- https://github.com/AliveToolkit/alive2, beside this directory and with translation validation ([see README](https://github.com/AliveToolkit/alive2#building-and-running-translation-validation)).
- https://github.com/avast/retdec, beside and built with `cmake .. -DCMAKE_INSTALL_PREFIX=$(pwd)/prefix -DRETDEC_DEV_TOOLS=1`
- https://github.com/lifting-bits/remill, as "remill" Docker container.

Usage:
- `tools/glue.sh 2100028b` performs the comparison on the opcode 2100028b. Output is printed to stdout and supplementary logs are written to /tmp.
- `tools/bulk.sh logs_dir` performs the comparison on many opcodes, calling glue.sh for each one. 
  - Progress is printed to stdout and comparison results (i.e. from glue.sh) are written to subfolders of logs_dir.
  - Opcodes are sourced from ../asl-interpreter/tests/coverage/\*, which has lists of opcodes liftable by the asl-interpreter.
- `tools/log_parser.py logs_dir out.csv` parses the log directory logs_dir which should contain the output of bulk.sh. Results are tabulated for further analysis.

Components:
- asl-translator/ contains an OCaml dune project which translates asl-interpreter's reduced ASL into LLVM IR. 
  - Dump ASL from asl-interpreter with `:dump A64 0x8b020021 /tmp/sem.aslb`, then run asl-translator with `dune exec test /tmp/sem.aslb`.
- This directory (llvm-translator) is a C++ project which unifies the LLVM IR from each lifter. 
  ```bash
  mkdir -p build && cd build 
  cmake ..
  make
  ./go rem /tmp/remill_out.ll  # also supports 'cap' and 'asl'
  ```
- tools/post.sh is used to post-process and simplify the output of llvm-translator before passing to alive. It calls opt and runs a given list of passes. 
- Further, Alive2 requires source/target to have the same set of global variables. llvm-translator supports `./go vars /tmp/cap.ll /tmp/rem.ll /tmp/asl.ll` which will union all variables mentioned by each lifter and insert them into the others.
- in/ and out/ contain old snapshots of LLVM code, as an example of the different LLVM IR styles from each lifter. in/ is directly from the lifter in question, and out/ is after (an old version of) llvm-translator.
