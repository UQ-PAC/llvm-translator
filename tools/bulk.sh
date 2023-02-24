#!/bin/bash


set -e
out="$1"
if [[ -z "$out" ]]; then
  echo "specify output directory as first argument"
  exit 1;
fi
pwd="$(pwd)"
mkdir -p "$out"
cd $(dirname $0)

pushd ../build
make
pushd ../asl-translator
dune build
popd
popd

export OCAMLRUNPARAM=b
set +e

rm -f ops

ASL=~/asl-interpreter
files="$(find "$ASL/tests/coverage" -maxdepth 1 -name 'aarch64_*')"
for f in $files;do
  echo $f
  mkdir -p "$pwd/$out/$(basename $f)"
  d=/tmp/$(date -I)
  mkdir -p $d
  pushd $ASL
  make || { echo "asli failed to build"; exit 1; }
  ops=$(grep -R ' --> OK' "$f" --no-filename | cut -d: -f1)
  echo "$ops" | sed -E "s#0x(..)(..)(..)(..)#:dump A64 0x\1\2\3\4 $d/\4\3\2\1.aslb#" | ./asli tests/override.asl tests/override.prj
  popd
  #echo "$ops" | sed 's/0x\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xargs -n 1 ./get_mnemonic.sh
  echo "$ops" | cat | sed 's/0x\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xargs -P 5 -i ./glue.sh '{}' "$pwd/$out/$(basename $f)" ';'
done
