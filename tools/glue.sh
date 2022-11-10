#!/bin/bash

cd $(dirname "$0")/..

ASLI=$HOME/asl-interpreter
ASL_TRANS=$HOME/llvm-translator/asl-translator
LLVM_TRANS=$HOME/llvm-translator

CAPSTONE=$HOME/retdec/build/prefix/bin/retdec-capstone2llvmir 

# all opcodes in little endian compressed format

function mnemonic() {
  op="$1"

  pushd $ASLI > /dev/null
  scripts/get_mnemonic.sh $op
  popd > /dev/null
}

function asli() {
  op="$1"
  f=$2

  a=${op:0:2}
  b=${op:2:2}
  c=${op:4:2}
  d=${op:6:2}
  hex="0x${d}${c}${b}${a}"

  pushd $ASLI
  scripts/get_mnemonic.sh $op
  echo :dump A64 $hex "$f" | ./asli tests/override.asl tests/override.prj
  x=$?
  popd

  return $x
}

function asl_translate() {
  in=$1
  out=$2
  pushd $ASL_TRANS > /dev/null
  _build/default/bin/main.exe $in > $out
  x=$?
  popd > /dev/null
  return $x
}

function capstone() {
  op=$1
  f=$2
  $CAPSTONE -a arm64 -b 0x0 -c "$op" -o $f
  return $?
}

function remill() {
  op=$1
  f=$2
  docker run remill --arch aarch64 --ir_out /dev/stdout --bytes "$op" > $f
  return $?
}

function llvm_translate() {
  in=$1
  out=$2
  mode=$3
  pushd $LLVM_TRANS > /dev/null
  build/go $mode $in > $out
  x=$?
  popd > /dev/null
  return $x
}

function alive() {
  a=${op:0:2}
  b=${op:2:2}
  c=${op:4:2}
  d=${op:6:2}
  hex="0x${d}${c}${b}${a}"

  echo '|' $hex
  echo '|' $1
  echo '|' $2

  ~/alive2/build/alive-tv --time-verify --smt-stats --bidirectional --disable-undef-input --disable-poison-input $1 $2
}

function main() {
  op=$1

  d=/tmp/$(date -I)
  mkdir -p $d

  aslb=$d/$op.aslb
  aslll=$d/$op.asl.ll
  cap=$d/$op.cap
  rem=$d/$op.rem

  capll=$d/$op.cap.ll
  remll=$d/$op.rem.ll

  alive=$d/$op.alive.out

  # asli $op $aslb 
  test -f $aslb || { echo "executing ASLI"; asli $op $aslb; }
  test -f $aslb || { echo "$op --> asli fail"; exit 1; }

  capstone $op $cap || { echo "$op --> capstone fail"; exit 2; }
  remill $op $rem   || { echo "$op --> remill fail"; exit 3; }

  asl_translate $aslb $aslll      || { echo "$op --> asl-translator fail"; exit 4; }
  llvm_translate $cap $capll cap  || { echo "$op --> llvm-translator cap fail"; exit 5; }
  llvm_translate $rem $remll rem  || { echo "$op --> llvm-translator rem fail"; exit 6; }

  rm -fv $alive
  mnemonic $op >> $alive
  alive $capll $aslll >> $alive
  echo ========================================== >> $alive
  alive $remll $aslll >> $alive

  grep --color=auto -E "seems to be correct|equivalent|reverse|doesn't verify|ERROR:|UB triggered|^[|] |^"$'\t' $alive | sed "s/^/$op --> /"

  echo $capll
  echo $remll 
  echo $aslll
  echo $alive
  correct=$(grep 'seems to be equivalent' $alive | wc -l)
  [[ $correct == 2 ]] || exit 100
}

main "$@"
exit $?
