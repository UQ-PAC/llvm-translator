#!/bin/bash

search() {
  name=$1
  shift
  for f in "$@"; do
    if [[ -x $f ]] && [[ ! -d $f ]]; then
      echo "$f"
      return 0
    elif command -v "$f" >/dev/null; then
      echo "$(command -v $f)"
      return 0
    fi
  done
  echo "warning: cannot find '$name'. tried: $@" >&2
  exit 1
}

search-d() {
  name=$1
  subdir=$2
  shift 2
  for f in "$@"; do
    if [[ -d "$f" ]] && [[ -e "$f/$subdir" ]]; then
      echo "$f"
      return 0
    fi
  done
  echo "warning: cannot find dir for '$name' with contents '$subdir'. tried: $@" >&2
  exit 1
}

DIR="$(dirname "${BASH_SOURCE[0]}")"/..
DIR="$(realpath "$DIR")"
export ASLI=$(search asli "$ASLI" $DIR/../asl-interpreter/asli aslp)
export ASL_TRANSLATOR=$(search asl-translator "$ASL_TRANSLATOR" $DIR/asl-translator/_build/default/bin/main.exe asl-translator)
export LLVM_TRANSLATOR=$(search llvm-translator "$LLVM_TRANSLATOR" $DIR/build/llvm-translator llvm-translator)
export CAPSTONE=$(search retdec-capstone2llvmir "$CAPSTONE" $DIR/../retdec/build/prefix/bin/retdec-capstone2llvmir retdec-capstone2llvmir)
export ALIVE=$(search alive-tv "$ALIVE" $DIR/../alive2/build/alive-tv alive-tv)
export ASLI_DIR=$(search-d asli tests/coverage "$ASLI_DIR" "$(dirname $ASLI)" ~/.nix-profile/share/asli)

for v in "$ASLI" "$ASL_TRANSLATOR" "$LLVM_TRANSLATOR" "$CAPSTONE" "$ALIVE" "$ASLI_DIR"; do
  if [[ -z "$v" ]]; then
    exit 1
  fi
done
