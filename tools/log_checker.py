#!/usr/bin/env python3 

import csv
import collections
import itertools

from dataclasses import dataclass
from itertools import chain, islice, repeat
from typing import Generic, Iterator, Literal, TypeVar, Callable

def main(argv):
  assert len(argv) >= 2, "requires csv file as second argument"
  assert len(argv) >= 3, "requires smaller csv file as third argument"
  larger = set()
  with open(argv[1], newline='') as f: 
    read = csv.DictReader(f)
    for r in read:
      larger.add(r['mnemonic'])
  smaller = set()
  with open(argv[2], newline='') as f: 
    read = csv.DictReader(f)
    for r in read:
      smaller.add(r['mnemonic'])
  print('\n'.join(sorted(larger - smaller)))

if __name__ == '__main__':
  import sys
  main(sys.argv)
