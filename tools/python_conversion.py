#!/usr/bin/env python3
"""Preview or apply Python 2-to-3 conversions selected by the source inventory."""

import ast
import pathlib
import subprocess
import tempfile

from conversion import main


def transform(source, root, relative, tool):
  """Convert legacy Python syntax and shebangs; leave Python 3 helpers alone."""
  first = source.splitlines()[0] if source else b''
  if first.startswith(b'#!') and b'python3' in first:
    ast.parse(source, filename=relative)
    return source, 'Already declares Python 3; skipped 2to3.'
  with tempfile.TemporaryDirectory(prefix='pcb-python-') as directory:
    target = pathlib.Path(directory) / pathlib.Path(relative).name
    target.write_bytes(source)
    result = subprocess.run([tool, '-w', '-n', str(target)],
                            capture_output=True, text=True)
    diagnostics = result.stdout + result.stderr
    if result.returncode:
      raise ValueError(diagnostics.strip())
    candidate = target.read_bytes()
    lines = candidate.splitlines(keepends=True)
    if lines and lines[0].startswith(b'#!') and b'python' in lines[0]:
      lines[0] = b'#!/usr/bin/env python3\n'
      candidate = b''.join(lines)
    ast.parse(candidate, filename=relative)
    return candidate, diagnostics

if __name__ == '__main__':
  raise SystemExit(main('python', transform, '2to3-3.12'))
