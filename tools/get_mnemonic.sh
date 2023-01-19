#!/bin/bash

[[ -z "$1" ]] && { echo "opcode required"; exit 1; }

if [[ "$1" == 0x* ]]; then
    # 0x opcodes from ASLI are in big-endian format
    a=${1:6:2}
    b=${1:4:2}
    c=${1:2:2}
    d=${1:0:2}
    op="0x$a 0x$b 0x$c 0x$d"
else
    # otherwise assume we have space separated bytes
    op=$(echo "$@" | tr -d ' ')
    d=${op:6:2}
    c=${op:4:2}
    b=${op:2:2}
    a=${op:0:2}
    op="0x$a 0x$b 0x$c 0x$d"
fi

echo "$op" | llvm-mc --arch arm64 -mattr=+crc,+flagm,+neon,+sve,+sve2,+dotprod,+fp-armv8,+sme,+v8.6a --disassemble | grep -v '.text'
