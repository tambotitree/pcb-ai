#!/usr/bin/env python3
"""Check inventoried Scheme with Guile 3; rewrite only established migration rules."""

import pathlib
import subprocess
import tempfile

from conversion import main


def transform(source, root, relative, tool):
  """Read every Scheme form without evaluating host-specific gEDA procedures."""
  version = subprocess.check_output([tool, '--version'], text=True)
  if 'Guile) 3.' not in version:
    raise ValueError('This check requires Guile 3')
  with tempfile.TemporaryDirectory(prefix='pcb-scheme-') as directory:
    target = pathlib.Path(directory) / pathlib.Path(relative).name
    target.write_bytes(source)
    program = '''(call-with-input-file (cadr (command-line))
      (lambda (port) (let loop ()
        (let ((form (read port)))
          (unless (eof-object? form) (loop))))))'''
    result = subprocess.run([tool, '--no-auto-compile', '-c', program, str(target)],
                            capture_output=True, text=True)
    if result.returncode:
      raise ValueError(result.stdout + result.stderr)
  return source, 'Guile 3 reader passed; host runtime APIs unverified. No rewrite rule needed.'


if __name__ == '__main__':
  raise SystemExit(main('scheme', transform, 'guile'))
