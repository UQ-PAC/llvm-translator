#!/bin/bash

cd $(dirname "$0")/..

ASLI=$HOME/asl-interpreter
ASL_TRANS=$HOME/llvm-translator/asl-translator
LLVM_TRANS=$HOME/llvm-translator

# note all opcodes in compressed little endian hex format.

function asli() {
  op="$1"
  f=$2

  a=${op:0:2}
  b=${op:2:2}
  c=${op:4:2}
  d=${op:6:2}
  hex="0x${d}${c}${b}${a}"

  pushd $ASLI
  echo :dump A64 $hex "$f" | ./asli tests/override.asl tests/override.prj
  x=$?
  popd

  return $x
}

function asl_translate() {
  in=$1
  out=$2
  pushd $ASL_TRANS
  _build/default/bin/main.exe $in > $out
  x=$?
  popd
  return $x
}

function capstone() {
  op=$1
  f=$2
  $HOME/retdec/build/prefix/bin/retdec-capstone2llvmir -a arm64 -b 0x0 -c $op -o $f
  return $?
}

function remill() {
  op=$1
  f=$2
  docker run -it remill --arch aarch64 --ir_out /dev/stdout --bytes "$op" > $f
  return $?
}

function llvm_translate() {
  in=$1
  out=$2
  mode=$3
  pushd $LLVM_TRANS
  build/go $mode $in > $out
  x=$?
  popd
  return $x
}

function alive() {
  ~/alive2/build/alive-tv --time-verify --smt-stats --bidirectional --disable-undef-input --disable-poison-input $1 $2
}

function main() {
  op=$1

  d=$(mktemp -d)

  aslb=$d/$op.aslb
  aslll=$d/$op.asl.ll
  cap=$d/$op.cap
  rem=$d/$op.rem

  capll=$d/$op.cap.ll
  remll=$d/$op.rem.ll

  alive=$d/alive.out

  set -ex
  asli $op $aslb

  capstone $op $cap
  remill $op $rem

  asl_translate $aslb $aslll
  llvm_translate $cap $capll cap
  llvm_translate $rem $remll rem

  alive $remll $aslll >> $alive
  echo ========================================== >> $alive
  alive $capll $aslll >> $alive
  set +ex
}

main "$@"
exit $?
