#!/bin/bash

cd $(dirname "$0")/..

ASLI=$HOME/asl-interpreter
ASL_TRANS=$HOME/llvm-translator/asl-translator
LLVM_TRANS=$HOME/llvm-translator

CAPSTONE=$HOME/retdec/build/prefix/bin/retdec-capstone2llvmir 

# all opcodes in little endian compressed format

function mnemonic() {
  op="$1"

  tools/get_mnemonic.sh $op
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
  build/go $mode $in 2>&1 1>${out}tmp
  x=$?
  cat ${out}tmp | tools/post.sh > $out 
  rm -f ${out}tmp
  popd > /dev/null
  return $x
}

function llvm_translate_vars() {
  pushd $LLVM_TRANS > /dev/null
  build/go vars $@
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

function prefix() {
  sed "s/^/$1 --> /"
}

function main() {
  op=$1

  d=/tmp/$(date -I)
  mkdir -p $d

  aslb=$d/$op.aslb
  asl=$d/$op.asl
  cap=$d/$op.cap
  rem=$d/$op.rem

  aslll=$d/$op.asl.ll
  capll=$d/$op.cap.ll
  remll=$d/$op.rem.ll

  alive=$d/$op.alive.out

  mnemonic $op | prefix $op 

  # asli $op $aslb 
  test -f $aslb || { echo "executing ASLI"; asli $op $aslb; }
  test -f $aslb || { echo "$op ==> asli fail"; exit 1; }

  capstone $op $cap || { echo "$op ==> capstone fail"; exit 2; }
  remill $op $rem   || { echo "$op ==> remill fail"; exit 3; }

  set -o pipefail
  asl_translate $aslb $asl | prefix $op       || { echo "$op ==> asl-translator fail"; exit 4; }
  llvm_translate $cap $capll cap | prefix $op || { echo "$op ==> llvm-translator cap fail"; exit 5; }
  llvm_translate $rem $remll rem | prefix $op || { echo "$op ==> llvm-translator rem fail"; exit 6; }
  llvm_translate $asl $aslll asl | prefix $op || { echo "$op ==> llvm-translator asl fail"; exit 7; }
  llvm_translate_vars $aslll $capll $remll    || { echo "$op ==> llvm-translator vars fail"; exit 8; }

  rm -f $alive
  mnemonic $op >> $alive
  alive $capll $aslll >> $alive
  echo ========================================== >> $alive
  alive $remll $aslll >> $alive

  grep --color=auto -E \
    "seems to be correct|equivalent|reverse|doesn't verify|ERROR:|UB triggered|^[|] |failed" $alive \
    | prefix $op

  echo $capll
  echo $remll 
  echo $aslll
  echo $alive
  correct=$(grep 'seem to be equivalent' $alive | wc -l)
  if [[ $correct == 2 ]]; then 
    echo "$op ==> SUCCESS"
  else
    echo "$op ==> FAIL" 
  fi
}

x="$(main "$@")"
echo "$x"
exec echo "$x" | grep -q 'SUCCESS'
