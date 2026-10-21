#!/usr/bin/env python3
"""Normalize the trailing declaration in PCB's shared object-field macro."""

from conversion import main


def transform(source, root, relative, tool):
  """Remove the macro's redundant terminator; callers supply the semicolon."""
  if relative != 'src/global.h':
    raise ValueError('This rule is defined only for src/global.h')
  text = source.decode()
  start = text.index('#define ANYOBJECTFIELDS')
  end = text.index('/* Lines, pads, and rats', start)
  block = text[start:end]
  old = 'FlagType\tFlags;'
  new = 'FlagType\tFlags'
  if block.count(old) == 1:
    block = block.replace(old, new)
  elif old not in block and block.count(new) == 1:
    return source, 'Object-field macro already normalized.'
  else:
    raise ValueError('Unexpected object-field macro definition')
  return (text[:start] + block + text[end:]).encode(), (
    'Removed one trailing macro semicolon; field names/types/order unchanged. '
    'Existing callers and ANYLINEFIELDS supply the terminator.')


if __name__ == '__main__':
  raise SystemExit(main('struct-macros', transform, 'python3'))
