#!/usr/bin/env python3
"""Convert a narrow set of GTK constructors in inventoried C sources."""

import re

from conversion import main

# Mask comments and literals so examples, messages and resource strings survive.
NONCODE = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                     re.DOTALL)
BOX = re.compile(r'\bgtk_([hv])box_new\s*\(\s*(?:FALSE|false|0)\s*,\s*'
                 r'([A-Za-z_][A-Za-z0-9_]*|[0-9]+)\s*\)')
SIMPLE = re.compile(r'\bgtk_([hv])(paned|separator)_new\s*\(\s*\)')


def transform(source, root, relative, tool):
  """Replace nonhomogeneous boxes, panes and separators; preserve other code."""
  if not relative.startswith('src/hid/gtk/'):
    return source, 'Outside GTK GUI; no mechanical rules selected.'
  text = source.decode('utf-8')
  masked = NONCODE.sub(lambda match: ' ' * len(match[0]), text)
  edits = []
  for match in BOX.finditer(masked):
    orientation = 'HORIZONTAL' if match[1] == 'h' else 'VERTICAL'
    edits.append((match.start(), match.end(),
                  'gtk_box_new (GTK_ORIENTATION_' + orientation + ', ' + match[2] + ')'))
  for match in SIMPLE.finditer(masked):
    orientation = 'HORIZONTAL' if match[1] == 'h' else 'VERTICAL'
    edits.append((match.start(), match.end(),
                  'gtk_' + match[2] + '_new (GTK_ORIENTATION_' + orientation + ')'))
  for start, end, replacement in sorted(edits, reverse=True):
    text = text[:start] + replacement + text[end:]
  return text.encode(), str(len(edits)) + ' constructor replacements; C compilation pending.'


if __name__ == '__main__':
  raise SystemExit(main('c', transform, 'python3'))
