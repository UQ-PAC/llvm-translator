#!/bin/bash 



cd $(dirname $0)

pushd ../build 
make
pushd ../asl-translator 
dune build 
popd
popd

ASL=~/asl-interpreter
for f in "aarch64_integer___"; do 
  echo $f
  d=/tmp/$(date -I)
  pushd $ASL
  make
  ops=$(grep -R ' --> OK' tests/coverage/$f --no-filename | cut -d: -f1)
  #echo "$ops" | sed -E "s#(0x[0-9a-f]{8})#:dump A64 \1 $d/\1.aslb#" | ./asli tests/override.asl tests/override.prj
  popd
  echo "$ops" | shuf | sed 's/0x\(..\)\(..\)\(..\)\(..\)/\4\3\2\1/' | xargs -P 4 -n 1 ./glue.sh 

done
