#!/bin/bash


# glue.sh [opcode] [output directory]
# opcode in little endian compressed format.
# output directory is optional,
# if given, log output is written to [opcode].{err,out}
# in that directory
#
# LLVM and ASL files are written to a subfolder of
# /tmp with the date. the subfolder path is logged.

d="$2"
if ! [[ -z "$d" ]]; then
  echo "$d/$1.out" "$d/$1.err" >&2
  exec 2>"$d/$1.err"
  exec 1>"$d/$1.out"
fi

cd $(dirname "$0")/..

. ./tools/env.sh


function mnemonic() {
  op="$1"
  ./tools/get_mnemonic.sh $op
}

function asli() {
  op="$1"
  f=$2

  a=${op:0:2}
  b=${op:2:2}
  c=${op:4:2}
  d=${op:6:2}
  hex="0x${d}${c}${b}${a}"

  echo :dump A64 $hex "$f" | "$ASLI"
  x=$?

  return $x
}

function asl_translate() {
  in=$1
  out=$2
  "$ASL_TRANSLATOR" $in > $out
  x=$?
  return $x
}

function capstone() {
  op=$1
  f=$2
  "$CAPSTONE" -a arm64 -b 0x0 -c "$op" -o $f
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

  "$LLVM_TRANSLATOR" $mode $in 2>&1 1>${out}tmp
  x=$?
  cat ${out}tmp | tools/post.sh > $out
  rm -f ${out}tmp

  return $x
}

function llvm_translate_vars() {
  "$LLVM_TRANSLATOR" vars $@
  x=$?
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

  "$ALIVE" --time-verify --smt-stats --bidirectional --disable-undef-input --disable-poison-input --smt-to=20000 $1 $2
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
  llvm_translate $cap $capll cap | prefix $op || { echo "$op ==> llvm-translator cap fail"; }
  llvm_translate $rem $remll rem | prefix $op || { echo "$op ==> llvm-translator rem fail"; }
  llvm_translate $asl $aslll asl | prefix $op || { echo "$op ==> llvm-translator asl fail"; exit 7; }
  llvm_translate_vars $aslll $capll $remll    || { echo "$op ==> llvm-translator vars fail"; exit 8; }

  rm -f ${alive}{.rem,.cap,}
  mnemonic $op >> $alive.cap
  mnemonic $op >> $alive.rem
  alive $capll $aslll >> $alive.cap
  alive $remll $aslll >> $alive.rem

  cat $alive.cap >> $alive
  echo ========================================== >> $alive
  cat $alive.rem >> $alive

  grep --color=auto -E \
    "seems to be correct|equivalent|reverse|doesn't verify|ERROR:|UB triggered|^[|] |failed" $alive \
    | prefix $op

  echo $capll
  echo $remll
  echo $aslll
  echo $alive
  cap=$(grep 'seem to be equivalent' $alive.cap | wc -l)
  rem=$(grep 'seem to be equivalent' $alive.rem | wc -l)
  if [[ $cap == 1 && $rem == 1 ]]; then
    echo "$op ==> SUCCESS. cap $cap, rem $rem"
  else
    echo "$op ==> FAILED. cap $cap, rem $rem"
  fi
}

x="$(main "$1")"
echo "$x"
exec echo "$x" | grep -q 'SUCCESS'
