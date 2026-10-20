#!/usr/bin/env python3
"""Preview typed replacements for simple K&R pointer-parameter definitions."""

import re

from c_conversion import NONCODE
from conversion import main

# Deliberately exclude scalar parameters: old-style default argument promotions
# need additional analysis. Never guess implicit int or complex declarators.
DEFINITION = re.compile(
  r'^(?P<result>REAL|double|float|int|void)\s+(?P<name>[A-Za-z_]\w*)'
  r'\((?P<names>[A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)*)\)\s*\n'
  r'(?P<declarations>(?:[ \t]*(?:const\s+)?[A-Za-z_]\w*\s+'
  r'\*+\s*[A-Za-z_]\w*\s*;[ \t]*\n)+)(?=\{)', re.MULTILINE)
PARAMETER = re.compile(r'\s*((?:const\s+)?[A-Za-z_]\w*)\s+(\*+)\s*([A-Za-z_]\w*)\s*;')


def transform(source, root, relative, tool):
  """Move existing explicit pointer declarations into the parameter list."""
  text = source.decode('utf-8')
  masked = NONCODE.sub(lambda match: ''.join('\n' if c == '\n' else ' '
                                            for c in match[0]), text)
  edits = []
  prototypes = []
  for match in DEFINITION.finditer(masked):
    # Do not consume comments or literals hidden by the lexical mask.
    if text[match.start():match.end()] != match[0]:
      continue
    names = [name.strip() for name in match['names'].split(',')]
    declarations = PARAMETER.findall(match['declarations'])
    by_name = {name: base + ' ' + pointer + name
               for base, pointer, name in declarations}
    if (len(names) != len(set(names)) or len(declarations) != len(names)
        or set(names) != set(by_name)):
      raise ValueError('inconsistent parameter declarations for ' + match['name'])
    signature = match['result'] + ' ' + match['name'] + ' ('
    signature += ', '.join(by_name[name] for name in names) + ')'
    prototypes.append(signature + ';')
    edits.append((match.start(), match.end(), signature + '\n'))
  for start, end, replacement in reversed(edits):
    text = text[:start] + replacement + text[end:]
  return text.encode(), (str(len(edits)) + ' signature replacements. Candidate declarations:\n'
                         + '\n'.join(prototypes)
                         + '\nExisting headers require compiler comparison; no header rewritten.')


if __name__ == '__main__':
  raise SystemExit(main('c-signatures', transform, 'python3'))
