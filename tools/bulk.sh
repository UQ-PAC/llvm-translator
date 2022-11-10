#!/bin/bash



set -e
cd $(dirname $0)

pushd ../build
make
pushd ../asl-translator
dune build
popd
popd

export OCAMLRUNPARAM=b
set +e

ASL=~/asl-interpreter
for f in "aarch64_integer___"; do
  echo $f
  d=/tmp/$(date -I)
  mkdir -p $d
  pushd $ASL
  make || { echo "asli failed to build"; exit 1; }
  ops=$(grep -R ' --> OK' tests/coverage/$f --no-filename | cut -d: -f1)
  echo "$ops" | sed -E "s#0x(..)(..)(..)(..)#:dump A64 0x\1\2\3\4 $d/\4\3\2\1.aslb#" | ./asli tests/override.asl tests/override.prj
  popd
  echo "$ops" | shuf | sed 's/0x\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xargs -P 5 -n 1 ./glue.sh

done
