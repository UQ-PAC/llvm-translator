#!/usr/bin/env python3 

from pathlib import Path
import os
import sys 
import argparse
import subprocess

import xml.etree.cElementTree as ET

mra_tools = Path.home() / 'mra_tools'
# print(mra_tools)
# sys.path.append(mra_tools / 'bin')

# import instrs2asl # type: ignore

PATTERNS = [
  'aarch64_integer.+', 
  # 'aarch64_branch.+',
  # 'aarch64_vector_arithmetic_unary_\(not\|rbit\|rev\|shift\|cnt\|clsz\)',
  # 'aarch64_vector_arithmetic_binary_.*_int.*',
  # 'aarch64_memory_single_general.*'
]

def main():
  for xml in (mra_tools / 'v8.6/ISA_A64_xml_v86A-2019-12/').iterdir():
    if not (xml.is_file() and xml.suffix == '.xml'):
      continue
    print(xml)
    xml = ET.parse(xml)
    print(xml.findall('.//instructionsection'))
    break



if __name__ == '__main__':
  main()
