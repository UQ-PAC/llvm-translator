#!/usr/bin/env python3 

# parses the stdout output of ./bulk.sh into a csv file for data analysis 



import csv
import collections
import itertools

from dataclasses import dataclass
from itertools import chain, islice, repeat
from typing import Generic, Iterator, Literal, TypeVar, Callable

T = TypeVar('T')

class PeekIterator(Generic[T]):
  EMPTY = object()

  def __init__(self, it):
    self.it = it 
    self.prev = self.EMPTY 
    self.repeat = False

  def __iter__(self):
    return self

  def __next__(self):
    if self.repeat:
      assert self.prev is not self.EMPTY
      self.repeat = False
      return self.prev

    self.prev = next(self.it)
    return self.prev

  def back(self):
    self.repeat = True
  

def forever() -> Iterator[None]:
  return repeat(None)

def takewhile(p, f):
  yield from itertools.takewhile(p, f)
  f.back()

def takeuntil(p: Callable[[T], bool], f: PeekIterator[T]) -> PeekIterator[T]:
  """Takes from the given iterator up to and including the first value that satisfies p."""
  yield from takewhile(lambda x: not p(x), f)
  yield next(f)


def consume(iterator, n=None) -> None:
  "Advance the iterator n-steps ahead. If n is None, consume entirely."
  # Use functions that consume iterators at C speed.
  if n is None:
    # feed the entire iterator into a zero-length deque
    collections.deque(iterator, maxlen=0)
  else:
    # advance to the empty slice starting at position n
    next(islice(iterator, n, n), None)

LifterResult = Literal['success', 'mismatch', 'ub', 'hypercall', 'timeout', 'domain', 'empty', 'unknown', 'variable']

@dataclass
class Result:
  opcode: str # little-endian
  mnemonic: str 
  category: str 
  asl: Literal['success', 'fail']
  cap_bool: bool = False
  rem_bool: bool = False
  cap: LifterResult = 'unknown'
  rem: LifterResult = 'unknown'
  detail: str = ''
  errors: str = ''

def get_result(block: str) -> tuple[bool, LifterResult]:
  if 'These functions seem to be equivalent!' in block: 
    return True, 'success'
  if 'Timeout' in block:
    return False, 'timeout' 
  if 'UB triggered' in block:
    return False, 'ub'
  if 'Mismatch' in block: 
    return False, 'mismatch'
  if 'return domain' in block:
    return False, 'domain'
  if block.count('\n') == 4 or '0 failed-to-prove' in block:
    return False, 'empty'
  if 'ERROR: ' in block:
    return False, 'unknown'
  print(repr(block))
  assert False, 'unhandled result block'  


def parse_op(f: PeekIterator[str], op: str) -> Result:
  x = next(f)
  if '\t' not in x:
    mnemonic = 'unknown'
    f.back()
  else:
    op_old = op
    op, name, args = x.split('\t')
    op = op.split()[0]
    assert op == op_old
    mnemonic = (name + ' ' + args).strip()

  consume(takeuntil(lambda x: 'Capstone' in x, f))

  l = list(takeuntil(lambda x: '==> FAILED' in x or '==> SUCCESS' in x, f))
  detail = ''.join(l)
  l = PeekIterator(iter(l))
  if 'asl-translator fail' in detail: 
    return Result(op, mnemonic, '', False, detail=detail)
  if 'llvm-translator vars fail' in detail: 
    return Result(op, mnemonic, '', False, detail=detail)

  head = ''.join(takewhile(lambda x: ' --> | ' not in x, l))
  hypercall = 'hyper_call' in head

  lifter_block = lambda: chain(
    takewhile(lambda x: ' --> | ' in x, l),
    takewhile(lambda x: ' --> | ' not in x and ' ==> ' not in x, l)
  )

  cap_block = ''.join(lifter_block())
  cap_bool,cap = get_result(cap_block)
  if 'unhandled capstone variable' in detail:
    cap = 'variable'

  rem_block = ''.join(lifter_block())
  rem_bool,rem = get_result(rem_block)
  if hypercall: rem = 'hypercall'

  return Result(op, mnemonic, '', True, (cap_bool), (rem_bool), cap, rem, detail)


def main(argv):
  assert len(argv) >= 2, "log directory required as first argument"
  assert len(argv) >= 3, "out file required as second argument"

  from pathlib import Path

  fname = argv[1]
  results = []
  for d in Path(fname).iterdir():
    if not d.is_dir(): continue 
    for f in d.iterdir():
      print(f)
      if not f.name.endswith('.out'): continue
      with open(f) as file: 
        x = parse_op(PeekIterator(iter(file)), f.stem)
        x.category = d.name
        x.errors = f.with_suffix('.err').read_text()

        results.append(x)

  print(len(results))

  with open(argv[2], 'w', newline='') as f:
    fields = list(results[0].__dict__.keys())
    w = csv.DictWriter(f, fields)
    w.writeheader()
    w.writerows((x.__dict__ for x in results))

if __name__ == '__main__':
  import sys
  main(sys.argv)
